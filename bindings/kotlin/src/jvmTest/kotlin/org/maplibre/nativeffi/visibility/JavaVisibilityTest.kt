package org.maplibre.nativeffi.visibility

import java.lang.classfile.AccessFlags
import java.lang.classfile.Attributes
import java.lang.classfile.ClassFile
import java.lang.classfile.ClassModel
import java.lang.classfile.MethodModel
import java.lang.reflect.AccessFlag
import java.nio.file.FileSystems
import java.nio.file.Path
import kotlin.io.path.isDirectory
import kotlin.io.path.listDirectoryEntries
import kotlin.io.path.readBytes
import kotlin.metadata.ClassKind
import kotlin.metadata.KmClass
import kotlin.metadata.KmDeclarationContainer
import kotlin.metadata.Visibility
import kotlin.metadata.declaresDefaultValue
import kotlin.metadata.isSecondary
import kotlin.metadata.jvm.KotlinClassMetadata
import kotlin.metadata.jvm.fieldSignature
import kotlin.metadata.jvm.getterSignature
import kotlin.metadata.jvm.setterSignature
import kotlin.metadata.jvm.signature
import kotlin.metadata.jvm.syntheticMethodForAnnotations
import kotlin.metadata.kind
import kotlin.metadata.visibility
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.RuntimeHandle

private const val PUBLIC_PACKAGE = "org/maplibre/nativeffi"
private const val PUBLISHED_API = "Lkotlin/PublishedApi;"

/**
 * Java sees a Kotlin `internal` declaration as public. A Java caller could otherwise forge a
 * handle, whose cleanup would then dispose the real one, or reach a handle's native state. So each
 * constructor, method, and field of the public package that javac can call must be one that other
 * Kotlin modules can call too. A member marked `@PublishedApi internal` is exempt, because that's
 * how the binding gives Java a member that Kotlin callers don't see.
 *
 * The internal packages stay reachable from Java, as they are through reflection, and their name
 * marks them as off limits.
 */
class JavaVisibilityTest {
  @Test
  fun javaCallsOnlyWhatOtherKotlinModulesCan() {
    val classes = bindingClasses()
    assertTrue(RuntimeHandle::class.java.name.replace('.', '/') in classes, "found no classes")
    assertEquals(
      emptyList(),
      classes.values.flatMap { it.leaks() }.sorted(),
      "Java can call these members, but other Kotlin modules can't. Mark an internal member " +
        "@JvmSynthetic, make a constructor private behind an internal factory, or mark a member " +
        "that only Java calls @PublishedApi.",
    )
  }
}

/** A compiled class of the public package, and the Kotlin declarations that it compiles. */
private class BindingClass(val model: ClassModel, private val classes: Map<String, BindingClass>) {
  val internalName: String = model.thisClass().asInternalName()

  private val metadata: KotlinClassMetadata? by lazy {
    Class.forName(internalName.replace('/', '.'), false, javaClass.classLoader)
      .getAnnotation(Metadata::class.java)
      ?.let(KotlinClassMetadata::readStrict)
  }

  private val kmClass: KmClass?
    get() = (metadata as? KotlinClassMetadata.Class)?.kmClass

  private val container: KmDeclarationContainer?
    get() =
      when (val metadata = metadata) {
        is KotlinClassMetadata.Class -> metadata.kmClass
        is KotlinClassMetadata.FileFacade -> metadata.kmPackage
        is KotlinClassMetadata.MultiFileClassPart -> metadata.kmPackage
        else -> null
      }

  /** This class's entry in its own InnerClasses attribute, which only a nested class has. */
  private val nesting by lazy {
    model.findAttribute(Attributes.innerClasses()).orElse(null)?.classes()?.firstOrNull {
      it.innerClass().asInternalName() == internalName
    }
  }

  private val outer: BindingClass?
    get() = nesting?.outerClass()?.orElse(null)?.let { classes[it.asInternalName()] }

  /** Whether Java source can name this class. It can't name an anonymous or local class. */
  private val javaVisible: Boolean by lazy {
    val nesting = nesting
    if (nesting == null) {
      model.flags().javaCallable()
    } else {
      nesting.innerName().isPresent &&
        (AccessFlag.PUBLIC in nesting.flags() || AccessFlag.PROTECTED in nesting.flags()) &&
        AccessFlag.SYNTHETIC !in nesting.flags() &&
        outer?.javaVisible == true
    }
  }

  /** Whether another Kotlin module can name this class. */
  private val kotlinVisible: Boolean by lazy {
    val own =
      when (metadata) {
        is KotlinClassMetadata.Class -> kmClass!!.visibility.published
        is KotlinClassMetadata.FileFacade,
        is KotlinClassMetadata.MultiFileClassFacade,
        is KotlinClassMetadata.MultiFileClassPart -> true
        else -> false
      }
    own && (nesting == null || outer?.kotlinVisible == true)
  }

  private val companion: BindingClass?
    get() = kmClass?.companionObject?.let { classes["$internalName$$it"] }

  /** The members that Java can call but other Kotlin modules can't. */
  fun leaks(): List<String> {
    if (!javaVisible) return emptyList()
    val members =
      model.methods().filter { it.javaCallable() }.map { it.signature() } +
        model
          .fields()
          .filter { it.flags().javaCallable() }
          .map { it.fieldName().stringValue() + ":" + it.fieldType().stringValue() }
          .filter { it !in objectFields() }
    val name = internalName.replace('/', '.')
    if (!kotlinVisible) return members.map { "$name.$it (class isn't public in Kotlin)" }
    // A companion's `@JvmStatic` methods and `const` and `@JvmField` fields compile to statics
    // of its outer class.
    val declarations = (companion?.declarations() ?: emptyMap()) + declarations()
    return members.mapNotNull { member ->
      val declaration = declarations[member] ?: generated(member)
      when {
        declaration == null -> "$name.$member (no Kotlin declaration)"
        declaration.visibility.published || declaration.publishedApi -> null
        else -> "$name.$member (${declaration.visibility.name.lowercase()})"
      }
    }
  }

  /**
   * The fields that hold an object: an object's `INSTANCE` and its companion's field. Java can read
   * them, but the object's own class decides what Java can call on it.
   */
  private fun objectFields(): Set<String> =
    setOfNotNull(
      "INSTANCE:L$internalName;".takeIf { kmClass?.kind == ClassKind.OBJECT },
      kmClass?.companionObject?.let { "$it:L$internalName$$it;" },
    )

  /** The JVM members that this class's Kotlin declarations compile to. */
  private fun declarations(): Map<String, Declaration> {
    val result = mutableMapOf<String, Declaration>()
    val container = container ?: return result
    kmClass?.constructors?.forEach { constructor ->
      val declaration = Declaration(constructor.visibility, publishedApi(constructor.signature))
      constructor.signature?.let { result[it.toString()] = declaration }
      // A primary constructor whose parameters all have defaults also compiles to a constructor
      // without parameters.
      if (!constructor.isSecondary && constructor.valueParameters.all { it.declaresDefaultValue }) {
        result["<init>()V"] = declaration
      }
    }
    container.functions.forEach { function ->
      val declaration = Declaration(function.visibility, publishedApi(function.signature))
      function.signature?.let { result[it.toString()] = declaration }
    }
    container.properties.forEach { property ->
      // A property keeps its annotations on a synthetic method of its own.
      val publishedApi = publishedApi(property.syntheticMethodForAnnotations)
      property.getterSignature?.let {
        result[it.toString()] = Declaration(property.getter.visibility, publishedApi)
      }
      property.setterSignature?.let {
        result[it.toString()] = Declaration(property.setter!!.visibility, publishedApi)
      }
      property.fieldSignature?.let {
        result["${it.name}:${it.descriptor}"] = Declaration(property.visibility, publishedApi)
      }
    }
    kmClass?.kmEnumEntries?.forEach {
      result["${it.name}:L$internalName;"] = Declaration(Visibility.PUBLIC)
    }
    return result
  }

  /** A member that the compiler adds without a declaration of its own. */
  private fun generated(member: String): Declaration? {
    val kmClass = kmClass ?: return null
    val public = Declaration(Visibility.PUBLIC)
    return when (member) {
      "toString()Ljava/lang/String;",
      "hashCode()I",
      "equals(Ljava/lang/Object;)Z" -> public
      "values()[L$internalName;",
      "valueOf(Ljava/lang/String;)L$internalName;",
      "getEntries()Lkotlin/enums/EnumEntries;" ->
        public.takeIf { kmClass.kind == ClassKind.ENUM_CLASS }
      else -> null
    }
  }

  private fun publishedApi(signature: Any?): Boolean =
    signature != null &&
      model.methods().any {
        it.signature() == signature.toString() && PUBLISHED_API in it.invisibleAnnotations()
      }
}

private class Declaration(val visibility: Visibility, val publishedApi: Boolean = false)

private val Visibility.published: Boolean
  get() = this == Visibility.PUBLIC || this == Visibility.PROTECTED

private fun MethodModel.signature() = methodName().stringValue() + methodType().stringValue()

private fun MethodModel.invisibleAnnotations(): List<String> =
  findAttribute(Attributes.runtimeInvisibleAnnotations())
    .map { attribute -> attribute.annotations().map { it.className().stringValue() } }
    .orElse(emptyList())

private fun MethodModel.javaCallable(): Boolean {
  val name = methodName().stringValue()
  return when {
    name == "<clinit>" -> false
    // A name such as `getId-s-VKNKU` isn't a Java identifier, so javac can't call it.
    name != "<init>" && !name.isJavaIdentifier() -> false
    else -> flags().javaCallable() && AccessFlag.BRIDGE !in flags().flags()
  }
}

private fun AccessFlags.javaCallable() =
  (has(AccessFlag.PUBLIC) || has(AccessFlag.PROTECTED)) && !has(AccessFlag.SYNTHETIC)

private fun String.isJavaIdentifier() =
  isNotEmpty() && first().isJavaIdentifierStart() && drop(1).all(Char::isJavaIdentifierPart)

/** The classes of the public package, by internal name, from wherever the test loads them. */
private fun bindingClasses(): Map<String, BindingClass> {
  val location = Path.of(RuntimeHandle::class.java.protectionDomain.codeSource.location.toURI())
  val files =
    if (location.isDirectory()) {
      location.resolve(PUBLIC_PACKAGE).listDirectoryEntries("*.class").map { it.readBytes() }
    } else {
      FileSystems.newFileSystem(location).use { jar ->
        jar.getPath(PUBLIC_PACKAGE).listDirectoryEntries("*.class").map { it.readBytes() }
      }
    }
  val classes = mutableMapOf<String, BindingClass>()
  for (bytes in files) {
    val compiled = BindingClass(ClassFile.of().parse(bytes), classes)
    classes[compiled.internalName] = compiled
  }
  return classes
}
