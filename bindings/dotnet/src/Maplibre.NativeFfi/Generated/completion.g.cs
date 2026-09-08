namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_command_disposition : uint
    {
        MLN_COMMAND_DISPOSITION_COMMITTED = 0,
        MLN_COMMAND_DISPOSITION_SUPERSEDED = 1,
        MLN_COMMAND_DISPOSITION_FAILED = 2,
        MLN_COMMAND_DISPOSITION_CANCELLED = 3,
    }

    internal unsafe partial struct mln_completion_result
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public mln_status status;

        [NativeTypeName("uint32_t")]
        public uint disposition;

        [NativeTypeName("uint32_t")]
        public uint reserved;

        [NativeTypeName("uint64_t")]
        public ulong generation;

        public mln_buffer_view diagnostic;

        [NativeTypeName("const void *")]
        public void* value;

        [NativeTypeName("size_t")]
        public nuint value_count;
    }

    internal unsafe partial struct mln_completion
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("mln_completion_callback")]
        public delegate* unmanaged[Cdecl]<void*, mln_completion_result*, void> callback;

        public void* user_data;

        [NativeTypeName("mln_completion_release")]
        public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    }
}
