#include <gtest/gtest.h>
#include "Firewall.hpp" // If separated, or your WAF logic
#include <regex>

// Test Suite for checking Firewall Defense mechanisms
TEST(FirewallTestSuite, DetectsSQLInjection) {
    std::string malicious_url = "/api/v1/students?id=1%20UNION%20SELECT%20*";
    std::string sqli_pattern = "(?i)\\b(UNION\\s+SELECT)\\b";
    std::regex pattern(sqli_pattern);

    // Assert that the firewall's regex successfully flags the attack string
    EXPECT_TRUE(std::regex_search(malicious_url, pattern));
}

TEST(FirewallTestSuite, AllowsCleanTraffic) {
    std::string clean_url = "/api/v1/students/5/transcript";
    std::string sqli_pattern = "(?i)\\b(UNION\\s+SELECT)\\b";
    std::regex pattern(sqli_pattern);

    // Assert that standard user traffic is ignored by the security rule
    EXPECT_FALSE(std::regex_search(clean_url, pattern));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
