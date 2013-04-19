<%
# Test for issue #10
# An array operation immediately after a method declaration
# confuses slash into thinking the [] method should operate
# on the previously declared Method.
# Link: https://github.com/slash-lang/slash/issues/10
class Issue10RegressionTest extends Test {
  def test_map_after_method_definition {
    assert_equal([1, 4, 9], \{
      def baloney {
        ...;
      }

      [1, 2, 3].map(\n { n * n});
    }.call);
  }
}
