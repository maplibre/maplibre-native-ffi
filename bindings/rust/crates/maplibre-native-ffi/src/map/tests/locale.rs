use super::*;

fn expression_map() -> (RuntimeHandle, MapHandle) {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
    (runtime, map)
}

fn assert_expression(map: &MapHandle, condition: JsonValue) {
    let expression = serde_json::to_vec(&json!(["case", condition, 1, 0])).unwrap();
    map.set_layer_property("background", "background-opacity", &expression)
        .unwrap();
    assert_eq!(
        map.layer_property("background", "background-opacity")
            .unwrap()
            .map(|bytes| serde_json::from_slice::<JsonValue>(&bytes).unwrap()),
        Some(json!(1.0)),
        "expression: {}",
        String::from_utf8(expression).unwrap()
    );
}

#[test]
fn linux_number_format_preserves_locale_and_fraction_options() {
    let (_runtime, map) = expression_map();
    for (number, options, expected) in [
        (1234.5678, json!({"locale": "en-US"}), "1,234.568"),
        (1234.5, json!({"locale": "de-DE"}), "1.234,5"),
        (
            1234.5,
            json!({"locale": "fr-FR", "min-fraction-digits": 2}),
            "1\u{202f}234,50",
        ),
        (
            12.3,
            json!({"locale": "en-US", "min-fraction-digits": 2, "max-fraction-digits": 4}),
            "12.30",
        ),
        (
            12.3,
            json!({"locale": "en-US", "min-fraction-digits": 4}),
            "12.3000",
        ),
        (
            1234.5,
            json!({"locale": "en-US-u-nu-arab", "max-fraction-digits": 0}),
            "١,٢٣٤",
        ),
        (-1234.5, json!({"locale": "en-US"}), "-1,234.5"),
        (
            0.0,
            json!({"locale": "en-US", "min-fraction-digits": 2}),
            "0.00",
        ),
    ] {
        assert_expression(
            &map,
            json!(["==", ["number-format", number, options], expected]),
        );
    }
}

#[test]
fn linux_number_format_currency_defaults_and_explicit_precision() {
    let (_runtime, map) = expression_map();
    for (number, options, expected) in [
        (
            1234.5,
            json!({"locale": "de-DE", "currency": "EUR"}),
            "1.234,50\u{a0}€",
        ),
        (
            1234.567,
            json!({"locale": "en-US", "currency": "USD", "max-fraction-digits": 1}),
            "$1,234.6",
        ),
        (
            12.3,
            json!({"locale": "en-US", "currency": "USD", "min-fraction-digits": 3}),
            "$12.300",
        ),
        (
            12.3,
            json!({"locale": "en-US", "currency": "USD", "max-fraction-digits": 0}),
            "$12",
        ),
        (
            -12.3,
            json!({"locale": "en-US", "currency": "USD"}),
            "-$12.30",
        ),
        (
            1234.6,
            json!({"locale": "ja-JP", "currency": "JPY"}),
            "￥1,235",
        ),
        (
            12.3,
            json!({"locale": "en-US", "currency": "KWD"}),
            "KWD\u{a0}12.300",
        ),
        (
            12.3,
            json!({"locale": "en-US", "currency": "ZZZ"}),
            "ZZZ\u{a0}12.30",
        ),
    ] {
        assert_expression(
            &map,
            json!(["==", ["number-format", number, options], expected]),
        );
    }
}

#[test]
fn linux_locale_errors_allow_later_style_changes() {
    let (_runtime, map) = expression_map();
    for invalid in [
        json!(["number-format", 12.3, {"locale": "en-US", "currency": "US"}]),
        json!(["number-format", 12.3, {"locale": "en-US", "min-fraction-digits": 4, "max-fraction-digits": 2}]),
        json!(["number-format", 12.3, {"locale": "not_a_tag"}]),
    ] {
        let expression =
            serde_json::to_vec(&json!(["case", ["==", invalid, "unused"], 1, 0])).unwrap();
        let error = map
            .set_layer_property("background", "background-opacity", &expression)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_expression(
            &map,
            json!(["==", ["number-format", 12.3, {"locale": "en-US"}], "12.3"]),
        );
    }
}
