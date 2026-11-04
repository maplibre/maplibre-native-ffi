// Ports maplibre-native-ffi src/resources/url_rules.{hpp,cpp}: the URL
// patterns of resource provider routes and HTTP header rules, a glob
// compiled into a linear NFA, and the validated copies of the caller's
// route and rule arrays.

use crate::diagnostics::Failure;
use crate::http_headers::{set_header, validate_http_header};
use crate::types::*;

const KNOWN_URL_MATCH_FLAGS: u32 = MLN_URL_MATCH_GLOB;
const MAX_GLOB_TOKENS: usize = 4096;

fn reject(what: &str, index: usize, reason: &str) -> Failure {
    Failure::invalid_argument(format!("{what} {index} {reason}"))
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum TokenKind {
    Literal(u8),
    /// `?`: one byte other than `/`.
    Any,
    /// `*`: any bytes other than `/`.
    Segment,
    /// `**`: any bytes.
    Spanning,
}

/// `CompiledGlob`. Each candidate byte visits each token once, so matching
/// is O(pattern length * candidate length).
pub struct CompiledGlob {
    tokens: Vec<TokenKind>,
}

impl CompiledGlob {
    /// `CompiledGlob::token_count`.
    pub fn token_count(pattern: &[u8]) -> usize {
        let mut count = 0;
        let mut index = 0;
        while index < pattern.len() {
            if pattern[index] == b'*' {
                while index + 1 < pattern.len() && pattern[index + 1] == b'*' {
                    index += 1;
                }
            } else if pattern[index] == b'\\' && index + 1 < pattern.len() {
                index += 1;
            }
            count += 1;
            index += 1;
        }
        count
    }

    pub fn new(pattern: &[u8]) -> CompiledGlob {
        let mut tokens = Vec::with_capacity(Self::token_count(pattern));
        let mut index = 0;
        while index < pattern.len() {
            let value = pattern[index];
            if value == b'*' {
                let start = index;
                while index + 1 < pattern.len() && pattern[index + 1] == b'*' {
                    index += 1;
                }
                tokens.push(if index == start { TokenKind::Segment } else { TokenKind::Spanning });
            } else if value == b'?' {
                tokens.push(TokenKind::Any);
            } else {
                if value == b'\\' && index + 1 < pattern.len() {
                    index += 1;
                }
                tokens.push(TokenKind::Literal(pattern[index]));
            }
            index += 1;
        }
        CompiledGlob { tokens }
    }

    pub fn matches(&self, candidate: &[u8]) -> bool {
        let wildcard = |kind: TokenKind| matches!(kind, TokenKind::Segment | TokenKind::Spanning);
        let mut states = vec![false; self.tokens.len() + 1];
        states[0] = true;
        for (index, &kind) in self.tokens.iter().enumerate() {
            states[index + 1] = wildcard(kind) && states[index];
        }
        for &byte in candidate {
            let mut previous = states[0];
            states[0] = false;
            for (index, &kind) in self.tokens.iter().enumerate() {
                let old = states[index + 1];
                states[index + 1] = match kind {
                    TokenKind::Segment | TokenKind::Spanning => {
                        states[index] || (old && (kind == TokenKind::Spanning || byte != b'/'))
                    }
                    TokenKind::Any => previous && byte != b'/',
                    TokenKind::Literal(literal) => previous && byte == literal,
                };
                previous = old;
            }
        }
        states[self.tokens.len()]
    }
}

/// `matches_kind`.
fn matches_kind(kinds: u32, request_kind: u32) -> bool {
    request_kind < 32 && (kinds >> request_kind) & 1 != 0
}

/// `UrlPattern`. Zero kinds select every kind.
pub struct UrlPattern {
    kinds: u32,
    url: Vec<u8>,
    glob: Option<CompiledGlob>,
}

impl UrlPattern {
    fn new(kinds: u32, flags: u32, url: &[u8]) -> UrlPattern {
        UrlPattern {
            kinds: if kinds == 0 { MLN_RESOURCE_KIND_FLAG_ALL } else { kinds },
            url: url.to_vec(),
            glob: (flags & MLN_URL_MATCH_GLOB != 0).then(|| CompiledGlob::new(url)),
        }
    }

    pub fn matches(&self, kind: u32, candidate: &[u8]) -> bool {
        matches_kind(self.kinds, kind)
            && match &self.glob {
                Some(glob) => glob.matches(candidate),
                None => self.url == candidate,
            }
    }
}

/// `RouteTable`: the routes of a resource provider. A table without routes
/// claims every request.
#[derive(Default)]
pub struct RouteTable {
    routes: Vec<UrlPattern>,
}

impl RouteTable {
    pub fn matches(&self, kind: u32, resolved_url: &str) -> bool {
        self.routes.is_empty() || self.routes.iter().any(|route| route.matches(kind, resolved_url.as_bytes()))
    }
}

/// One header rule: the requests it covers and the headers it adds.
pub struct HeaderRule {
    pattern: UrlPattern,
    headers: Vec<(String, String)>,
}

/// `HeaderRuleTable`.
#[derive(Default)]
pub struct HeaderRuleTable {
    rules: Vec<HeaderRule>,
}

/// `validate_c_string_view`: text that reaches a C string consumer keeps
/// its full length.
pub fn validate_c_string_view<'a>(view: mln_buffer_view, name: &str) -> Result<&'a [u8], Failure> {
    // SAFETY: C API contract for buffer views.
    let Some(bytes) = (unsafe { view.bytes() }) else {
        return Err(Failure::invalid_argument(format!("{name} has null data with a nonzero size")));
    };
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument(format!("{name} contains embedded NUL")));
    }
    Ok(bytes)
}

fn check_pattern<'a>(
    what: &str,
    index: usize,
    kinds: u32,
    flags: u32,
    url: mln_buffer_view,
) -> Result<&'a [u8], Failure> {
    let Ok(url) = validate_c_string_view(url, "url") else {
        return Err(reject(what, index, "has an invalid url"));
    };
    if kinds & !MLN_RESOURCE_KIND_FLAG_ALL != 0 {
        return Err(reject(what, index, "has unknown kind bits"));
    }
    if flags & !KNOWN_URL_MATCH_FLAGS != 0 {
        return Err(reject(what, index, "has unknown flag bits"));
    }
    if flags & MLN_URL_MATCH_GLOB != 0 && CompiledGlob::token_count(url) > MAX_GLOB_TOKENS {
        return Err(reject(what, index, "exceeds the glob limit of 4096 tokens"));
    }
    Ok(url)
}

/// `next_sized_element`: the element at `cursor`, stepping by its declared
/// size so a newer caller's larger elements are stepped over. `None` when the
/// element is smaller than this library's struct or its size would misalign
/// the next one.
///
/// # Safety
/// `cursor` points at a readable element whose first field is `u32 size`,
/// followed by `size` readable bytes.
unsafe fn next_sized_element<T: Copy>(cursor: &mut *const u8) -> Option<T> {
    // SAFETY: the caller's contract; `size` is the first field.
    let size = unsafe { *cursor.cast::<u32>() } as usize;
    if size < size_of::<T>() || !size.is_multiple_of(align_of::<T>()) {
        return None;
    }
    // SAFETY: as above.
    let element = unsafe { *cursor.cast::<T>() };
    // SAFETY: the caller's contract.
    *cursor = unsafe { cursor.add(size) };
    Some(element)
}

/// `copy_routes`.
///
/// # Safety
/// `routes` points at `count` routes, as the C API documents.
pub unsafe fn copy_routes(routes: *const mln_resource_route, count: usize) -> Result<RouteTable, Failure> {
    if routes.is_null() && count != 0 {
        return Err(Failure::invalid_argument("routes must not be null with a nonzero route_count"));
    }
    let mut table = RouteTable::default();
    let mut cursor = routes.cast::<u8>();
    for index in 0..count {
        // SAFETY: the caller's contract.
        let Some(route) = (unsafe { next_sized_element::<mln_resource_route>(&mut cursor) }) else {
            return Err(reject("route", index, "has an invalid size"));
        };
        let url = check_pattern("route", index, route.kinds, route.flags, route.url)?;
        table.routes.push(UrlPattern::new(route.kinds, route.flags, url));
    }
    Ok(table)
}

/// `copy_header_rules`.
///
/// # Safety
/// `rules` points at `count` rules, as the C API documents.
pub unsafe fn copy_header_rules(rules: *const mln_http_header_rule, count: usize) -> Result<HeaderRuleTable, Failure> {
    if rules.is_null() && count != 0 {
        return Err(Failure::invalid_argument("rules must not be null with a nonzero rule_count"));
    }
    let mut table = HeaderRuleTable::default();
    let mut cursor = rules.cast::<u8>();
    for index in 0..count {
        // SAFETY: the caller's contract.
        let Some(rule) = (unsafe { next_sized_element::<mln_http_header_rule>(&mut cursor) }) else {
            return Err(reject("header rule", index, "has an invalid size"));
        };
        let url = check_pattern("header rule", index, rule.kinds, rule.flags, rule.url)?;
        if rule.headers.is_null() && rule.header_count != 0 {
            return Err(reject("header rule", index, "has null headers with a nonzero header_count"));
        }
        let mut headers = Vec::with_capacity(rule.header_count);
        for header in 0..rule.header_count {
            // SAFETY: the caller's contract.
            let header = unsafe { *rule.headers.add(header) };
            // The diagnostic names the header, never its value.
            let (name, value) = validate_http_header(header.name, header.value)
                .map_err(|failure| Failure::invalid_argument(format!("header rule {index}: {}", failure.message)))?;
            headers.push((String::from_utf8_lossy(name).into_owned(), String::from_utf8_lossy(value).into_owned()));
        }
        table.rules.push(HeaderRule { pattern: UrlPattern::new(rule.kinds, rule.flags, url), headers });
    }
    Ok(table)
}

impl HeaderRuleTable {
    /// `header_rules_callback`: the first rule that matches adds its headers.
    pub fn apply(&self, kind: u32, url: &str, headers: &mut Vec<(String, String)>) {
        if let Some(rule) = self.rules.iter().find(|rule| rule.pattern.matches(kind, url.as_bytes())) {
            for (name, value) in &rule.headers {
                set_header(headers, name, value);
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::CompiledGlob;

    fn glob(pattern: &str, candidate: &str) -> bool {
        CompiledGlob::new(pattern.as_bytes()).matches(candidate.as_bytes())
    }

    #[test]
    fn globs_match_as_the_header_documents() {
        assert!(glob("https://*.example.com/tiles/**", "https://a.example.com/tiles/1/2/3.pbf"));
        assert!(!glob("https://*.example.com/tiles/**", "https://a.b.example.com/x/tiles/1"));
        assert!(glob("file://**", "file:///var/mobile/style.json"));
        assert!(!glob("a?c", "a/c"));
        assert!(glob("a\\*c", "a*c"));
        assert!(!glob("a\\*c", "abc"));
        assert!(glob("", ""));
        assert!(!glob("", "x"));
    }
}
