<%
# Test for issue #10
# NoMethodError: Undefined method '[]' on #<Method:0x2348bc8>
# https://github.com/slash-lang/slash/issues/10

class Issue10RegressionTest extends Test {
  def test_map_after_method_definition {
    assert_equal([1, 4, 9], \{
      def baloney {
        ...;
      }

      [1, 2, 3].map(\n { n * n});
    }.call);
  }
}.register;
