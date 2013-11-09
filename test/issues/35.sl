<%
# When casefolding unicode characters in String#upper or String#lower, it's
# possible that the UTF-8 representation of the folded character may be a
# different byte length to the original character.
class Issue35RegressionTest extends Test {
  def test_angstrom_sign_lower {
    assert_equal("\uE5", "\u212B".lower);
  }
}
