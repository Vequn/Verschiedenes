import re
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Final, List, Optional, Tuple, Set

# ==========================================
# 1. IMMUTABLE DOMAIN MODELS & SCHEMAS
# ==========================================

@dataclass(frozen=True)
class ValidationResult:
    """Immutable data record encapsulating complete validation telemetry."""
    is_valid: bool
    issuer_name: str
    luhn_passed: bool
    length_passed: bool
    sanitized_length: int


# ==========================================
# 2. STRATEGY PATTERN FOR ISSUER ROUTING
# ==========================================

class IssuerStrategy(ABC):
    """Abstract Base Strategy representing an ISO/IEC 7812 Issuer Rule."""
    
    @property
    @abstractmethod
    def name(self) -> str:
        pass

    @property
    @abstractmethod
    def allowed_lengths(self) -> Set[int]:
        pass

    @abstractmethod
    def matches(self, sanitized_number: str) -> bool:
        """Determines if the IIN/BIN matches this issuer profile."""
        pass


class PrefixIssuerStrategy(IssuerStrategy):
    """Optimized Strategy for exact prefix matching using compiled regex."""
    def __init__(self, name: str, prefixes: List[str], allowed_lengths: List[int]):
        self._name: Final[str] = name
        self._lengths: Final[Set[int]] = set(allowed_lengths)
        # Compile prefixes into a single optimized regex OR pattern
        pattern = f"^({ '|'.join(prefixes) })"
        self._regex: Final[re.Pattern] = re.compile(pattern)

    @property
    def name(self) -> str: return self._name

    @property
    def allowed_lengths(self) -> Set[int]: return self._lengths

    def matches(self, sanitized_number: str) -> bool:
        return bool(self._regex.match(sanitized_number))


class RangeIssuerStrategy(IssuerStrategy):
    """Strategy for complex variable-length numerical BIN ranges (e.g., Discover/Mastercard)."""
    def __init__(self, name: str, ranges: List[Tuple[int, int]], prefix_len: int, allowed_lengths: List[int]):
        self._name: Final[str] = name
        self._ranges: Final[List[Tuple[int, int]]] = ranges
        self._prefix_len: Final[int] = prefix_len
        self._lengths: Final[Set[int]] = set(allowed_lengths)

    @property
    def name(self) -> str: return self._name

    @property
    def allowed_lengths(self) -> Set[int]: return self._lengths

    def matches(self, sanitized_number: str) -> bool:
        if len(sanitized_number) < self._prefix_len:
            return False
        extracted_prefix = int(sanitized_number[:self._prefix_len])
        # Binary search could be used here if ranges list grew massive; linear scan sufficient for standard sets
        return any(low <= extracted_prefix <= high for low, high in self._ranges)


# ==========================================
# 3. CORE VALIDATION ENGINE (CORE PROCESSOR)
# ==========================================

class AdvancedCreditCardValidator:
    """
    High-performance, stateless validator Engine.
    Thread-safe and optimized for batch transaction pipelines.
    """
    # Pre-compiled global sanitization regex to prevent catastrophic backtracking and overhead
    _SANIZATION_REGEX: Final[re.Pattern] = re.compile(r"[\s-]")
    _NUMERIC_ONLY_REGEX: Final[re.Pattern] = re.compile(r"^\d+$")

    # Luhn optimization lookup table: eliminates multiplication/conditional branching inside the hot loop.
    # Represents: (index * 2) if under 10 else (index * 2 - 9) for indices 0-9
    _LUHN_LOOKUP: Final[Tuple[int, ...]] = (0, 2, 4, 6, 8, 1, 3, 5, 7, 9)

    def __init__(self):
        """Initialize and bootstrap issuer routing protocols."""
        self._strategies: Final[List[IssuerStrategy]] = [
            PrefixIssuerStrategy("Visa", ["4"], [13, 16, 19]),
            PrefixIssuerStrategy("American Express", ["34", "37"], [15]),
            PrefixIssuerStrategy("Mastercard", [str(i) for i in range(51, 56)], [16]),
            RangeIssuerStrategy("Mastercard", [(2221, 2720)], 4, [16]),
            PrefixIssuerStrategy("Discover", ["6011", "65"] + [str(i) for i in range(644, 650)], [16, 19]),
            RangeIssuerStrategy("Discover", [(622126, 622926)], 6, [16, 19]),
            PrefixIssuerStrategy("Maestro", ["5018", "5020", "5038", "5893", "6304", "6759", "6761", "6762", "6763"], list(range(12, 20)))
        ]

    def sanitize(self, raw_input: str) -> str:
        """Removes formatting characters using pre-compiled regex operations."""
        return self._SANIZATION_REGEX.sub("", raw_input)

    def execute_luhn(self, sanitized_number: str) -> bool:
        """
        An optimized implementation of the Luhn Checksum.
        Time Complexity: O(N) linear scan
        Space Complexity: O(1) auxiliary space (no array allocation or string reversal arrays).
        """
        total_sum = 0
        # Iterate backwards without allocating new reversed string arrays
        parity = 0
        
        for i in range(len(sanitized_number) - 1, -1, -1):
            digit = int(sanitized_number[i])
            if parity & 1:  # Bitwise parity check (every 2nd digit from right)
                total_sum += self._LUHN_LOOKUP[digit]
            else:
                total_sum += digit
            parity += 1
            
        return (total_sum % 10) == 0

    def validate(self, raw_number: str) -> ValidationResult:
        """
        Orchestrates full structural and algorithmic checks.
        Guaranteed non-throwing; returns comprehensive immutable state telemetry.
        """
        if not isinstance(raw_number, str):
            return ValidationResult(False, "Malformed Input Data", False, False, 0)

        sanitized = self.sanitize(raw_number)
        sanitized_len = len(sanitized)

        # Fail early on structurally non-numeric or empty strings
        if sanitized_len == 0 or not self._NUMERIC_ONLY_REGEX.match(sanitized):
            return ValidationResult(False, "Malformed Input Data", False, False, sanitized_len)

        # Phase 1: Structural Metadata Discovery (Routing Engine)
        matched_issuer = "Unknown Issuer"
        length_passed = False
        
        for strategy in self._strategies:
            if strategy.matches(sanitized):
                matched_issuer = strategy.name
                length_passed = sanitized_len in strategy.allowed_lengths
                break
        else:
            # Fallback range logic for compliance with global base standards (ISO 7812 lengths)
            length_passed = 12 <= sanitized_len <= 19

        # Phase 2: Mathematical Invariant Checks
        luhn_passed = self.execute_luhn(sanitized)

        # Final Evaluation Aggregator
        is_valid = length_passed and luhn_passed

        return ValidationResult(
            is_valid=is_valid,
            issuer_name=matched_issuer,
            luhn_passed=luhn_passed,
            length_passed=length_passed,
            sanitized_length=sanitized_len
        )
