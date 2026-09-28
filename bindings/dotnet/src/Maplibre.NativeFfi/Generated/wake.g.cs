namespace Maplibre.NativeFfi.Internal.C
{
    internal unsafe partial struct mln_wake
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("mln_wake_callback")]
        public delegate* unmanaged[Cdecl]<void*, void> callback;

        public void* user_data;

        [NativeTypeName("mln_wake_release")]
        public delegate* unmanaged[Cdecl]<void*, void> release_user_data;
    }
}
