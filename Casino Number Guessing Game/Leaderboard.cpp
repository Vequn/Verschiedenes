#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <ctime>   
#include <limits>  
#include <fstream> 
#include <algorithm> // Needed for sorting the leaderboard vector

using namespace std;

// Struct to represent an entry on the global leaderboard
struct LeaderboardEntry {
    string name;
    int score;
};

// Comparator function to sort leaderboard entries in descending order
bool compareEntries(const LeaderboardEntry& a, const LeaderboardEntry& b) {
    return a.score > b.score;
}

// ============================================================================
// PLAYER CLASS
// ============================================================================
class Player {
private:
    string name;
    int balance;
    int currentBet;
    int currentGuess;
    bool active;
    bool banned;          
    int securityStrikes;  

public:
    Player(string p_name, int p_balance, bool p_banned = false) 
        : name(p_name), balance(p_balance), currentBet(0), currentGuess(0), 
          active(!p_banned), banned(p_banned), securityStrikes(0) {}

    string getName() const { return name; }
    int getBalance() const { return balance; }
    int getBet() const { return currentBet; }
    int getGuess() const { return currentGuess; }
    bool isActive() const { return active; }
    bool isBanned() const { return banned; }
    int getStrikes() const { return securityStrikes; }

    void setInactive() { active = false; }
    
    void flagViolation() {
        securityStrikes++;
        if (securityStrikes >= 3) {
            banned = true;
            active = false;
        }
    }

    bool placeBet(int amt) {
        if (amt <= 0 || amt > balance) {
            return false; 
        }
        currentBet = amt;
        return true;
    }

    bool makeGuess(int g, int maxNum) {
        if (g < 1 || g > maxNum) {
            return false;
        }
        currentGuess = g;
        return true;
    }

    void updateBalance(bool won, int payoutMultiplier) {
        if (won) {
            balance += (currentBet * payoutMultiplier);
        } else {
            balance -= currentBet;
        }
        if (balance <= 0) {
            active = false; 
        }
    }
};

// ============================================================================
// CASINO ENGINE CLASS WITH ANTI-CHEAT & LEADERBOARD
// ============================================================================
class AdvancedCasino {
private:
    const int MAX_NUMBER = 10;
    const int PAYOUT_MULTIPLIER = 10;
    const string SAVE_EXT = "_save.txt";
    const string LEADERBOARD_FILE = "casino_leaderboard.txt"; // Master Leaderboard File

    vector<Player> tablePlayers; 
    int winningNumber;

    void clearInput() const {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    void saveSystem(const Player& p) {
        ofstream file(p.getName() + SAVE_EXT);
        if (file.is_open()) {
            if (p.isBanned()) {
                file << "BANNED"; 
            } else {
                file << p.getBalance();
            }
            file.close();
        }
        
        // If the player isn't banned and still has chips, update the master leaderboard file
        if (!p.isBanned() && p.getBalance() > 0) {
            updateLeaderboardFile(p.getName(), p.getBalance());
        }
    }

    bool loadSystem(const string& name, int& balance, bool& isBanned) {
        ifstream file(name + SAVE_EXT);
        if (file.is_open()) {
            string fileContent;
            file >> fileContent;
            file.close();

            if (fileContent == "BANNED") {
                isBanned = true;
                balance = 0;
                return true;
            } else {
                isBanned = false;
                balance = stoi(fileContent);
                return true;
            }
        }
        return false; 
    }

    // Leaderboard File Writer: Updates or adds a highscore record
    void updateLeaderboardFile(const string& name, int chips) {
        vector<LeaderboardEntry> entries;
        ifstream inFile(LEADERBOARD_FILE);
        bool found = false;

        // Read existing entries
        if (inFile.is_open()) {
            string existingName;
            int existingScore;
            while (inFile >> existingName >> existingScore) {
                if (existingName == name) {
                    // Update only if the new balance is higher than their previous record
                    if (chips > existingScore) {
                        existingScore = chips;
                    }
                    found = true;
                }
                entries.push_back({existingName, existingScore});
            }
            inFile.close();
        }

        // If it's a new player, append them to our local working list
        if (!found) {
            entries.push_back({name, chips});
        }

        // Write the sorted updated data back to the central data file
        ofstream outFile(LEADERBOARD_FILE);
        if (outFile.is_open()) {
            for (const auto& entry : entries) {
                outFile << entry.name << " " << entry.score << "\n";
            }
            outFile.close();
        }
    }

public:
    AdvancedCasino() : winningNumber(0) {
        srand(static_cast<unsigned int>(time(0))); 
    }

    // Fetches, sorts, and prints the top players dynamically onto the screen
    void displayLeaderboard() {
        vector<LeaderboardEntry> entries;
        ifstream file(LEADERBOARD_FILE);

        if (file.is_open()) {
            string name;
            int score;
            while (file >> name >> score) {
                entries.push_back({name, score});
            }
            file.close();
        }

        cout << "\n============================================================\n";
        cout << "             🏆 GLOBAL CASINO LEADERBOARD 🏆               \n";
        cout << "============================================================\n";
        
        if (entries.empty()) {
            cout << "  No records found yet. Be the first to secure a highscore!\n";
        } else {
            // Sort elements dynamically using our standard comparator
            sort(entries.begin(), entries.end(), compareEntries);

            int rank = 1;
            for (const auto& entry : entries) {
                cout << "  Rank " << rank << " | " << entry.name << " - $" << entry.score << "\n";
                rank++;
                if (rank > 5) break; // Limit viewport rendering to the top 5 players
            }
        }
        cout << "============================================================\n\n";
    }

    void registerPlayers() {
        cout << "============================================================\n";
        cout << "                 🎰 OOP CASINO MAIN HALL 🎰                 \n";
        cout << "============================================================\n";
        
        displayLeaderboard(); // Show current world rankings right at launch screen

        int count;
        while (true) {
            cout << "🎰 How many players want to join the table? (1-5): ";
            if (cin >> count && count >= 1 && count <= 5) break;
            cout << "❌ Invalid choice! Enter an integer between 1 and 5.\n";
            clearInput();
        }
        clearInput();

        for (int i = 0; i < count; ++i) {
            string name;
            cout << "👤 Enter Name for Player " << i + 1 << ": ";
            getline(cin, name);

            int loadedBalance = 0;
            bool isBanned = false;

            if (loadSystem(name, loadedBalance, isBanned)) {
                if (isBanned) {
                    cout << "   🚨 [SECURITY] ACCESS DENIED! User '" << name << "' is permanently banned.\n";
                    Player bannedPlayer(name, 0, true);
                    tablePlayers.push_back(bannedPlayer);
                } else {
                    cout << "   [Profile Found] Welcome back, " << name << "! Chips: $" << loadedBalance << "\n";
                    tablePlayers.push_back(Player(name, loadedBalance));
                }
            } else {
                int startBal;
                while (true) {
                    cout << "   " << name << ", enter starting chip balance ($): ";
                    if (cin >> startBal && startBal > 0) break;
                    cout << "   ❌ Invalid amount! Enter a positive cash value.\n";
                    clearInput();
                }
                clearInput();
                Player newPlayer(name, startBal);
                tablePlayers.push_back(newPlayer);
                saveSystem(newPlayer);
            }
        }
    }

    void playRound() {
        char nextRound = 'y';

        while (nextRound == 'Y' || nextRound == 'y') {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif

            cout << "============================================================\n";
            cout << "                 🟢 CASINO DEALER TABLE 🟢                  \n";
            cout << "============================================================\n";

            int activeCount = 0;

            for (auto& player : tablePlayers) {
                if (player.isBanned()) {
                    cout << "\n🚫 " << player.getName() << " is blacklisted and cannot participate.\n";
                    continue;
                }
                if (!player.isActive()) continue;
                
                activeCount++;
                cout << "\n👉 TURN: " << player.getName() << " | Current Chips: $" << player.getBalance() << "\n";

                // Bet Segment
                int bet;
                while (true) {
                    cout << "   Enter bet amount (Or press '0' to cash out): $";
                    if (cin >> bet) {
                        if (bet == 0) {
                            player.setInactive();
                            cout << "   👋 " << player.getName() << " has cashed out.\n";
                            break;
                        }
                        if (player.placeBet(bet)) break;
                    }
                    
                    player.flagViolation();
                    cout << "   ⚠️ [SECURITY WARNING] Invalid bet! Strikes: " << player.getStrikes() << "/3\n";
                    clearInput();

                    if (player.isBanned()) {
                        cout << "   🚨 [BAN] " << player.getName() << " has been permanently BANNED for manipulation!\n";
                        saveSystem(player);
                        break;
                    }
                }

                if (!player.isActive()) continue; 

                // Guess Segment
                int guess;
                while (true) {
                    cout << "   Guess the lucky number (1-" << MAX_NUMBER << "): ";
                    if (cin >> guess) {
                        if (player.makeGuess(guess, MAX_NUMBER)) break;
                    }
                    
                    player.flagViolation();
                    cout << "   ⚠️ [SECURITY WARNING] Out-of-bounds input! Strikes: " << player.getStrikes() << "/3\n";
                    clearInput();

                    if (player.isBanned()) {
                        cout << "   🚨 [BAN] " << player.getName() << " has been permanently BANNED for exploitation!\n";
                        saveSystem(player);
                        break;
                    }
                }
            }

            if (activeCount == 0) {
                cout << "\n🛑 Session terminated. No active players left.\n";
                break;
            }

            // Spinner execution
            winningNumber = rand() % MAX_NUMBER + 1;
            cout << "\n\n===============================================\n";
            cout << "🎲 BALL IS ROLLING... WINNING NUMBER IS: [" << winningNumber << "] 🎲\n";
            cout << "===============================================\n\n";

            // Process payouts and update ledger records
            for (auto& player : tablePlayers) {
                if (player.isBanned() || !player.isActive()) continue; 
                if (player.getBet() == 0) continue; 

                bool won = (player.getGuess() == winningNumber);
                player.updateBalance(won, PAYOUT_MULTIPLIER);

                if (won) {
                    cout << "🎉 WINNER! " << player.getName() << " won $" << player.getBet() * PAYOUT_MULTIPLIER << "!\n";
                } else {
                    cout << "❌ MISSED! " << player.getName() << " lost their bet of $" << player.getBet() << ".\n";
                }

                saveSystem(player); // Sync with player system file AND update the leaderboard highscores

                if (player.getBalance() <= 0) {
                    cout << "💀 BANKRUPT! " << player.getName() << " has run out of funds.\n";
                }
            }

            cout << "\n--> Should the dealer spin another round? (y/n): ";
            cin >> nextRound;
        }

        // Before closing the game program, show the refreshed global leaderboard rankings
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        displayLeaderboard();
        cout << "💰 Session Ended. Database files fully sync'd. Goodbye!\n";
    }
};

// ============================================================================
// MAIN ENTRY
// ============================================================================
int main() {
    AdvancedCasino table;
    table.registerPlayers();
    table.playRound();
    return 0;
}
