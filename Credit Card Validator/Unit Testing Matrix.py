def run_test_suite():
    engine = AdvancedCreditCardValidator()
    
    # Dataset Struct: (Raw String, Expected Validity, Expected Issuer)
    test_matrix = [
        # Standard Valid Cards
        ("4532 7153 9022 1367", True, "Visa"),
        ("3782-822463-10005", True, "American Express"),
        ("5105 1051 0510 5100", True, "Mastercard"),
        ("2221  0000  0000  0004", True, "Mastercard"), # 2221 Series check
        ("6011-1111-1111-1117", True, "Discover"),
        
        # Edge Case Failures
        ("4532 7153 9022 1368", False, "Visa"),              # Correct length/prefix, Luhn Checksum failure
        ("3782-822463-1000", False, "American Express"),     # Valid format prefix, truncated invalid length
        ("5105 1051 0510 510A", False, "Malformed Input Data"),# Alphabet injections
        ("", False, "Malformed Input Data"),                 # Empty payload
        ("1234-5678-1234-5670", False, "Unknown Issuer"),    # Standard ISO length, passes Luhn, unregistered prefix
    ]
    
    print(f"{'Raw Input Passed':<26} | {'Identified Issuer':<18} | {'Luhn':<5} | {'Len':<5} | {'Status':<5}")
    print("="*72)
    
    all_passed = True
    for raw, expected_valid, expected_issuer in test_matrix:
        res = engine.validate(raw)
        
        # Internal self-checking assertion framework
        status_marker = "PASS" if res.is_valid == expected_valid and res.issuer_name == expected_issuer else "FAIL"
        if status_marker == "FAIL":
            all_passed = False
            
        print(f"{raw:<26} | {res.issuer_name:<18} | {str(res.luhn_passed):<5} | {str(res.length_passed):<5} | {status_marker}")
        
    print("="*72)
    print(f"VERDICT: {'SYSTEM COMPLIANT' if all_passed else 'REGRESSION FOUND'}")

if __name__ == "__main__":
    run_test_suite()
