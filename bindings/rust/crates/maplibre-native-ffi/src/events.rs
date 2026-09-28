/// Identity for a map owned by a runtime. The value is the map's native handle,
/// which names one map for the life of the process. It carries no ownership;
/// map operations go through [`MapHandle`](crate::MapHandle).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct MapId(u64);

impl MapId {
    pub(crate) const fn new(value: u64) -> Self {
        Self(value)
    }

    /// Returns the numeric map identity.
    pub const fn get(self) -> u64 {
        self.0
    }
}
