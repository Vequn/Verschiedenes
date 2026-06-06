# --- Execution / Testing Block ---
if __name__ == "__main__":
    validator = CreditCardValidator()
    
    test_cases = [
        # Valid test cases (Simulated standard numbers)
        "4532 7153 9022 1367",       # Valid Visa
        "3782-822463-10005",         # Valid Amex with hyphens
        "5105 1051 0510 5100",       # Valid Mastercard
        
        # Intentionally invalid cases
        "4532 7153 9022 1368",       # Visa with wrong check digit (Luhn failure)
        "3782-822463-1000",          # Amex missing a digit (Length/Luhn failure)
        "not_a_card_1234"            # Malformed text
    ]
    
    print(f"{'Input Number':<25} | {'Issuer':<16} | {'Length':<6} | {'Luhn':<6} | {'Valid?':<6}")
    print("-" * 73)
    
    for case in test_cases:
        res = validator.validate(case)
        print(f"{case:<25} | "
              f"{res['issuer']:<16} | "
              f"{str(res['length_valid']):<6} | "
              f"{str(res['luhn_valid']):<6} | "
              f"{str(res['is_valid']):<6}")
