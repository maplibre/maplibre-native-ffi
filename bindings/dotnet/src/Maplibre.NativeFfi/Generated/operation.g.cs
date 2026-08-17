using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_poll([NativeTypeName("mln_operation")] MlnOperation operation, bool* out_completed);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_wait([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("int64_t")] long timeout_ms, bool* out_completed);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_cancel([NativeTypeName("mln_operation")] MlnOperation operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_get_status([NativeTypeName("mln_operation")] MlnOperation operation, mln_status* out_status);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_copy_diagnostic([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("char *")] sbyte* out_diagnostic, [NativeTypeName("size_t")] nuint diagnostic_capacity, [NativeTypeName("size_t *")] nuint* out_diagnostic_size);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_operation_finish([NativeTypeName("mln_operation")] MlnOperation operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_operation_release([NativeTypeName("mln_operation")] MlnOperation operation);
    }
}
