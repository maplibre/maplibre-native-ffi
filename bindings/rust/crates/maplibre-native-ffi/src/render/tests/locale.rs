use super::*;

struct ExpressionFixture {
    session: RenderSessionHandle,
    _context: OwnedTextureTestContext,
    _map: MapHandle,
    _runtime: RuntimeHandle,
}

impl ExpressionFixture {
    fn new() -> Self {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
        let (context, session) = create_owned_texture_session(
            &map.attach_ref().unwrap(),
            RenderTargetExtent::new(64, 64, 1.0),
        )
        .unwrap();
        load_query_style(&mut runtime, &map, &session);
        wait_for_source_feature(
            &mut runtime,
            &session,
            "point",
            &SourceFeatureQueryOptions::default(),
            "locale expression feature",
        );
        Self {
            session,
            _context: context,
            _map: map,
            _runtime: runtime,
        }
    }
}

fn assert_expression(session: &RenderSessionHandle, condition: JsonValue) {
    let mut options = SourceFeatureQueryOptions::default();
    options.filter = Some(serde_json::to_vec(&condition).unwrap());
    let features = session
        .query_source_features("point", Some(&options))
        .unwrap();
    assert_eq!(features.len(), 1, "expression: {condition}");
}

fn collator(locale: &str, case: bool, accent: bool) -> JsonValue {
    json!(["collator", {
        "locale": locale, "case-sensitive": case, "diacritic-sensitive": accent
    }])
}

#[test]
fn linux_collator_honors_sensitivity_and_unicode() {
    let fixture = ExpressionFixture::new();
    let session = &fixture.session;
    for case in [false, true] {
        for accent in [false, true] {
            let comparison = collator("en-US", case, accent);
            assert_expression(session, json!(["==", ["==", "A", "a", comparison], !case]));
            assert_expression(
                session,
                json!(["==", ["==", "é", "e", comparison], !accent]),
            );
            assert_expression(session, json!(["==", "é", "e\u{301}", comparison]));
        }
    }
    assert_expression(
        session,
        json!([">", "ä", "z", collator("sv-SE", false, false)]),
    );
    assert_expression(
        session,
        json!(["<", "ä", "z", collator("de-DE", false, false)]),
    );
    assert_expression(
        session,
        json!(["==", "İ", "i", collator("tr-TR", false, true)]),
    );
    assert_expression(
        session,
        json!(["<", "😀\u{0}a", "😀\u{0}b", collator("en-US", true, true)]),
    );
    assert_expression(
        session,
        json!(["<", "file2", "file10", collator("en-US-u-kn", true, true)]),
    );
    assert_expression(
        session,
        json!([
            ">",
            "file2",
            "file10",
            collator("en-US-u-kn-false", true, true)
        ]),
    );
    assert_expression(
        session,
        json!(["<", "A", "a", collator("en-US-u-kf-upper", true, true)]),
    );
    assert_expression(
        session,
        json!([
            "==",
            "ä",
            "ae",
            collator("de-DE-u-co-phonebk", false, false)
        ]),
    );
}

#[test]
fn linux_resolved_locale_reports_collation_fallback_and_extensions() {
    let fixture = ExpressionFixture::new();
    let session = &fixture.session;
    for (requested, expected) in [
        ("sv-SE", "sv"),
        ("de-DE-u-co-phonebk", "de-u-co-phonebk"),
        ("en-US-u-co-foobar", "en-US"),
        ("en-US-u-co-phonebk", "en-US"),
        ("en-US-u-kn-nu-arab", "en-US-u-kn"),
        ("en-US-u-kf-foobar-kn-foobar", "en-US"),
        ("iw-IL", "he"),
    ] {
        assert_expression(
            session,
            json!([
                "==",
                ["resolved-locale", collator(requested, true, true)],
                expected
            ]),
        );
    }
}

#[test]
fn linux_locale_expressions_evaluate_feature_inputs_and_recover_from_errors() {
    let fixture = ExpressionFixture::new();
    let session = &fixture.session;
    let condition = json!(["all",
        ["==", ["get", "kind"], "CÁPITAL", collator("en-US", false, false)],
        ["==", ["number-format", ["length", ["get", "kind"]], {
            "locale": ["case", ["get", "visible"], "de-DE", "en-US"],
            "min-fraction-digits": 2
        }], "7,00"]
    ]);
    let filter = serde_json::to_vec(&condition).unwrap();
    let mut rendered = RenderedFeatureQueryOptions::default();
    rendered.layer_ids = Some(vec!["point-circle".into()]);
    rendered.filter = Some(filter);
    for _ in 0..3 {
        assert_expression(session, condition.clone());
        let features = session
            .query_rendered_features(
                &RenderedQueryGeometry::point(ScreenPoint::new(32.0, 32.0)),
                Some(&rendered),
            )
            .unwrap();
        assert_eq!(features.len(), 1);
    }
    for invalid in [
        json!(["==", ["number-format", 1, {"currency": ["get", "kind"]}], "unused"]),
        json!([
            "==",
            ["get", "kind"],
            "capital",
            collator("not_a_tag", false, false)
        ]),
    ] {
        let mut options = SourceFeatureQueryOptions::default();
        options.filter = Some(serde_json::to_vec(&invalid).unwrap());
        assert!(
            session
                .query_source_features("point", Some(&options))
                .unwrap()
                .is_empty()
        );
        assert_expression(session, condition.clone());
    }
}

#[test]
fn linux_locale_defaults_follow_operation_categories() {
    const CHILD: &str = "MLN_FFI_LOCALE_TEST_CHILD";
    if let Ok(expected) = std::env::var(CHILD) {
        let fixture = ExpressionFixture::new();
        let session = &fixture.session;
        let (collation, number) = expected.split_once('|').unwrap();
        assert_expression(
            session,
            json!([
                "==",
                ["slice", ["resolved-locale", ["collator", {}]], 0, 2],
                collation
            ]),
        );
        assert_expression(
            session,
            json!(["==", ["number-format", 1234.5, {}], number]),
        );
        assert_expression(
            session,
            json!(["==", ["number-format", 1234.5, {"locale": "xx-ZZ"}], number]),
        );
        assert_expression(
            session,
            json!([
                "==",
                [
                    "slice",
                    ["resolved-locale", collator("xx-ZZ", false, false)],
                    0,
                    2
                ],
                collation
            ]),
        );
        assert_expression(
            session,
            json!(["==", ["number-format", 1234.5, {"locale": "en-US"}], "1,234.5"]),
        );
        return;
    }
    // Separate processes keep locale environment changes isolated from Rust and render threads.
    for (all, numeric, collation, language, expected) in [
        ("sv_SE.UTF-8", "de_DE", "de_DE", "en_US", "sv|1\u{a0}234,5"),
        (
            "",
            "fr_FR.UTF-8",
            "de_DE.UTF-8",
            "en_US",
            "de|1\u{202f}234,5",
        ),
        ("C.UTF-8", "fr_FR", "sv_SE", "de_DE", "en|1,234.5"),
        ("xx_ZZ", "fr_FR", "sv_SE", "de_DE", "en|1,234.5"),
        ("", "", "", "de_DE.UTF-8", "de|1.234,5"),
    ] {
        let output = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "render::tests::locale::linux_locale_defaults_follow_operation_categories",
                "--exact",
                "--nocapture",
            ])
            .env(CHILD, expected)
            .env("LC_ALL", all)
            .env("LC_NUMERIC", numeric)
            .env("LC_COLLATE", collation)
            .env("LANG", language)
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}\n{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
    }
}
