from locust import HttpUser, task, between
import random

class FinancialEngineStressTester(HttpUser):
    # Simulate realistic delay thinking thresholds between 0.1 to 0.5 seconds
    wait_time = between(0.1, 0.5)

    # Production validation test pools
    CARD_DATA_POOL = [
        "4532 7153 9022 1367",  # Valid Visa
        "5105 1051 0510 5100",  # Valid Mastercard
        "3782-822463-10005",    # Valid Amex
        "4532 7153 9022 1368",  # Invalid Checksum Card
    ]

    @task(1)
    def trigger_card_validation_endpoint(self):
        """Floods validation vectors asynchronously into backend routing systems."""
        payload = {
            "card_number": random.choice(self.CARD_DATA_POOL)
        }
        
        # Track response metrics and prevent logging flood overheads
        self.client.post("/api/v1/validate", json=payload, name="Validation Ingest")
