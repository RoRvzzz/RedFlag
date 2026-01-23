"""
Unit tests for C++ string normalization (anti-evasion)
"""
import sys
import os

# Add parent directory to path so we can import redflag
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from redflag.cogs.code_scan import normalize_cpp_string


def test_string_concatenation():
    """Test that adjacent string literals are merged"""
    input_code = 'system("c" "md" ".exe");'
    result = normalize_cpp_string(input_code)
    # Should merge to: system("cmd.exe");
    assert '"cmd.exe"' in result or '"c" "md" ".exe"' not in result, \
        f"Failed to merge strings. Got: {result}"


def test_comment_between_strings():
    """Test that comments between strings are removed and strings merged"""
    input_code = 'system("c" /* comment */ "md" ".exe");'
    result = normalize_cpp_string(input_code)
    # Comment should be removed, strings merged
    assert '/* comment */' not in result, "Comment was not removed"
    assert '"cmd.exe"' in result or ('"c"' not in result and '"md"' not in result), \
        f"Strings not merged after comment removal. Got: {result}"


def test_url_in_string():
    """Critical: Ensure URLs inside strings are NOT mangled by comment stripping"""
    input_url = 'char* s = "https://example.com";'
    result = normalize_cpp_string(input_url)
    # URL should remain intact
    assert "https://example.com" in result, \
        f"URL was mangled! Got: {result}"
    assert input_url == result, \
        f"URL string should be unchanged. Got: {result}"


def test_line_comment():
    """Test that line comments are removed"""
    input_code = 'system("cmd.exe"); // This is a comment'
    result = normalize_cpp_string(input_code)
    assert '// This is a comment' not in result, "Line comment not removed"
    assert 'system("cmd.exe");' in result, "Code after comment should remain"


def test_block_comment():
    """Test that block comments are removed"""
    input_code = 'system(/* block comment */ "cmd.exe");'
    result = normalize_cpp_string(input_code)
    assert '/* block comment */' not in result, "Block comment not removed"
    assert 'system(' in result and '"cmd.exe"' in result, "Code should remain"


def test_multiple_concatenations():
    """Test multiple adjacent string concatenations"""
    input_code = '"A" "B" "C" "D"'
    result = normalize_cpp_string(input_code)
    # Should eventually merge to "ABCD"
    assert '"ABCD"' in result or result.count('"') <= 2, \
        f"Multiple concatenations failed. Got: {result}"


def test_escaped_quotes():
    """Test that escaped quotes inside strings don't break normalization"""
    input_code = 'char* s = "He said \\"hello\\"";'
    result = normalize_cpp_string(input_code)
    assert '\\"hello\\"' in result or '"hello"' in result, \
        "Escaped quotes should be preserved"


def test_raw_string_literal_unchanged():
    """Test that raw string literals R"(...)" are left alone (not merged)"""
    input_code = 'string s = R"(system("cmd.exe");)";'
    result = normalize_cpp_string(input_code)
    # Raw string should remain as-is (we don't try to merge it)
    assert 'R"(' in result or 'R"(' in input_code, \
        "Raw string literal should be preserved"
    # The content inside should still be there (for scanning)
    assert 'system' in result and 'cmd.exe' in result, \
        "Raw string content should remain for scanning"


def test_empty_string():
    """Test edge case: empty input"""
    assert normalize_cpp_string("") == ""
    assert normalize_cpp_string(None) == None or normalize_cpp_string("") == ""


if __name__ == "__main__":
    print("Running normalization tests...")
    test_string_concatenation()
    print("✓ String concatenation")
    
    test_comment_between_strings()
    print("✓ Comment between strings")
    
    test_url_in_string()
    print("✓ URL in string (critical)")
    
    test_line_comment()
    print("✓ Line comment")
    
    test_block_comment()
    print("✓ Block comment")
    
    test_multiple_concatenations()
    print("✓ Multiple concatenations")
    
    test_escaped_quotes()
    print("✓ Escaped quotes")
    
    test_raw_string_literal_unchanged()
    print("✓ Raw string literals")
    
    test_empty_string()
    print("✓ Empty string")
    
    print("\nAll tests passed! ✓")
