plugins { `kotlin-dsl` }

repositories {
  google()
  mavenCentral()
}

dependencies {
  implementation(gradleApi())
  implementation("com.android.tools.build:gradle:${libs.versions.agp.get()}")
  implementation("com.vanniktech:gradle-maven-publish-plugin:${libs.versions.maven.publish.get()}")
  implementation("org.jetbrains.kotlin:kotlin-gradle-plugin:${libs.versions.kotlin.get()}")
}
