# Generated declarations

ClangSharp-generated C declarations will live in this directory. Keep generated
constants, layouts, opaque pointer types, and raw functions internal to the
binding.

The current scaffold uses a small handwritten `LibraryImport` slice in
`Internal/C/NativeMethods.cs` until the ClangSharp generation task lands.
