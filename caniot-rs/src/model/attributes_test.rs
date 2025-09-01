use super::attributes::*;

#[test]
fn test_attributes() {
    assert_eq!(Attribute::try_from(0x0000).unwrap(), Attribute::NodeId);

    for part in 0..=0xf {
        assert_eq!(
            Attribute::try_from(0x0050 + part).unwrap(),
            Attribute::BuildCommit
        );
    }
}
