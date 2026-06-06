import unittest
from validator import AdvancedCreditCardValidator, ValidationResult


class TestAdvancedCreditCardValidator(unittest.TestCase):
    """
    Academic & Production grade Test Suite for AdvancedCreditCardValidator.
    Validates boundary conditions, algorithmic integrity, and security invariants.
    """

    def setUp(self) -> None:
        """Bootstrap the testing fixture environment before running each test case."""
        self.engine = AdvancedCreditCardValidator()

    def test_sanitization_engine(self) -> None:
        """Ensures formatting characters are completely stripped without mutating data integrity."""
        dirty_inputs = [
            ("4532 7153 9022 1367", "4532715390221367"),
            ("3782-822463-10005", "378282246310005"),
            ("\t 6011\n-1111-1111-1117 ", "6011111111111117"),
        ]
        for raw, expected in dirty_inputs:
            with self.subTest(raw=raw):
                self.assertEqual(self.engine.sanitize(raw), expected)

    def test_valid_issuers_and_checksums(self) -> None:
        """
        Parametrized validation matrix testing structural properties and Luhn compliance
        across major global networks (ISO/IEC 7812).
        """
        # Schema: (Raw Input, Expected Issuer)
        valid_matrix = [
            ("4532 7153 9022 1367", "Visa"),
            ("3782-822463-10005", "American Express"),
            ("5105 1051 0510 5100", "Mastercard"),
            ("2221 0000 0000 0004", "Mastercard"),  # Lower bound of Mastercard 2017 BIN range
            ("6011-1111-1111-1117", "Discover"),
            ("5018 0000 0000 0004", "Maestro"),
        ]

        for raw, expected_issuer in valid_matrix:
            with self.subTest(issuer=expected_issuer, raw=raw):
                result: ValidationResult = self.engine.validate(raw)
                
                self.assertTrue(result.is_valid, f"Failed validity invariant for {expected_issuer}")
                self.assertEqual(result.issuer_name, expected_issuer)
                self.assertTrue(result.luhn_passed)
                self.assertTrue(result.length_passed)

    def test_luhn_checksum_failures(self) -> None:
        """Verifies that cards matching valid prefix and length specifications fail if the checksum digit is corrupt."""
        invalid_checksum_matrix = [
            "4532 7153 9022 1368",  # Visa with corrupt check digit
            "5105 1051 0510 5101",  # Mastercard with corrupt check digit
            "3782-822463-10004",    # Amex with corrupt check digit
        ]
        for raw in invalid_checksum_matrix:
            with self.subTest(raw=raw):
                result = self.engine.validate(raw)
                self.assertFalse(result.is_valid)
                self.assertFalse(result.luhn_passed)
                self.assertTrue(result.length_passed)  # Structural rules pass, mathematical check fails

    def test_length_boundary_constraints(self) -> None:
        """Asserts that cards with valid prefixes fail early if they break length rules."""
        invalid_length_matrix = [
            "4532 7153 9022 13",    # Visa truncated to 14 digits (Visa requires 13, 16, 19)
            "3782-822463-100",      # Amex truncated to 13 digits (Amex requires 15)
        ]
        for raw in invalid_length_matrix:
            with self.subTest(raw=raw):
                result = self.engine.validate(raw)
                self.assertFalse(result.is_valid)
                self.assertFalse(result.length_passed)

    def test_malformed_and_malicious_inputs(self) -> None:
        """Defensive boundary analysis targeting non-numeric, malicious, or empty injections."""
        malicious_inputs = [
            ("", "Malformed Input Data"),                   # Empty String
            ("4532 7153 9022 136A", "Malformed Input Data"),# Alphanumeric Injection
            ("4532; DROP TABLE cards;--", "Malformed Input Data"), # SQL Injection vector test
            ("   \n\t  ", "Malformed Input Data"),         # Only whitespaces
            (None, "Malformed Input Data"),                 # Invalid Type Injection
            (1234567812345670, "Malformed Input Data")      # Type Integer instead of String
        ]
        for raw, expected_err in malicious_inputs:
            with self.subTest(raw=raw):
                result = self.engine.validate(raw)  # type: ignore
                self.assertFalse(result.is_valid)
                self.assertEqual(result.issuer_name, expected_err)


if __name__ == "__main__":
    unittest.main(verbosity=2)
