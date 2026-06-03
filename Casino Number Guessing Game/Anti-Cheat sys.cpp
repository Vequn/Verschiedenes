#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <ctime>   
#include <limits>  
#include <fstream> 

using namespace std;

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
    bool banned;          // Anti-Cheat: Flag to lock out banned profiles
    int securityStrikes;  // Anti-Cheat: Counter for malicious attempts

public:
    Player(string p_name, int p_balance, bool p_banned = false) 
        : name(p_name), balance(p_balance), currentBet(0), currentGuess(0), 
          active(!p_banned), banned(p_banned), securityStrikes(0) {}

    // Getters
    string getName() const { return name; }
    int getBalance() const { return balance; }
    int getBet() const { return currentBet; }
    int getGuess() const { return currentGuess; }
    bool isActive() const { return active; }
    bool isBanned() const { return banned; }
    int getStrikes() const { return securityStrikes; }

    // Setters & State Modifiers
    void setInactive() { active = false; }
    
    void flagViolation() {
        securityStrikes++;
        if (securityStrikes >= 3) {
            banned = true;
            active = false;
        }
    }

    bool placeBet(int amt) {
        // Anti-Cheat: Catching integer overflow or negative manipulation attacks
        if (amt <= 0 || amt > balance) {
            return false; 
        }
        currentBet = amt;
        return true;
    }

    bool makeGuess(int g, int maxNum) {
        // Anti-Cheat: Strict bounds verification
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
// CASINO ENGINE CLASS WITH ANTI-CHEAT ENGINE
// ============================================================================
class AdvancedCasino {
private:
    const int MAX_NUMBER = 10;
    const int PAYOUT_MULTIPLIER = 10;
    const string SAVE_EXT = "_save.txt";

    vector<Player> tablePlayers; 
    int winningNumber;

    void clearInput() const {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Save System (Saves the balance OR a ban marker)
    void saveSystem(const Player& p) {
        ofstream file(p.getName() + SAVE_EXT);
        if (file.is_open()) {
            if (p.isBanned()) {
                file << "BANNED"; // Write permanent ban string to file
            } else {
                file << p.getBalance();
            }
            file.close();
        }
    }

    // Load System (Detects if the file contains a ban signature)
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

public:
    AdvancedCasino() : winningNumber(0) {
        srand(static_cast<unsigned int>(time(0))); 
    }

    void registerPlayers() {
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
                    cout << "   🚨 [SECURITY ALERT] ACCESS DENIED! User '" << name << "' is permanently BANNED for hacking.\n";
                    Player bannedPlayer(name, 0, true);
                    tablePlayers.push_back(bannedPlayer);
                } else {
                    cout << "   [Profile Found] Welcome back, " << name << "! Balance: $" << loadedBalance << "\n";
                    tablePlayers.push_back(Player(name, loadedBalance));
                }
            } else {
                int startBal;
                while (true) {
                    cout << "   " << name << ", enter your starting chip balance ($): ";
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

            // Phase 1: Inputs & Exploitation Scans
            for (auto& player : tablePlayers) {
                if (player.isBanned()) {
                    cout << "\n🚫 " << player.getName() << " is blacklisted and cannot participate.\n";
                    continue;
                }
                if (!player.isActive()) continue;
                
                activeCount++;
                cout << "\n👉 TURN: " << player.getName() << " | Current Chips: $" << player.getBalance() << "\n";

                // Bet Segment with Security Audit
                int bet;
                while (true) {
                    cout << "   Enter bet amount (Or press '0' to cash out): $";
                    if (cin >> bet) {
                        if (bet == 0) {
                            player.setInactive();
                            cout << "   👋 " << player.getName() << " has cashed out.\n";
                            break;
                        }
                        if (player.placeBet(bet)) {
                            break; // Valid input passed
                        }
                    }
                    
                    // Trigger security strike for bad data inputs
                    player.flagViolation();
                    cout << "   ⚠️ [SECURITY WARNING] Malicious/Invalid bet detected! Strikes: " << player.getStrikes() << "/3\n";
                    clearInput();

                    if (player.isBanned()) {
                        cout << "   🚨 [BAN ENGINE] " << player.getName() << " has been permanently BANNED for suspicious input manipulation!\n";
                        saveSystem(player);
                        break;
                    }
                }

                if (!player.isActive()) continue; 

                // Guess Segment with Security Audit
                int guess;
                while (true) {
                    cout << "   Guess the lucky number (1-" << MAX_NUMBER << "): ";
                    if (cin >> guess) {
                        if (player.makeGuess(guess, MAX_NUMBER)) {
                            break; // Valid input passed
                        }
                    }
                    
                    player.flagViolation();
                    cout << "   ⚠️ [SECURITY WARNING] Out-of-bounds manipulation detected! Strikes: " << player.getStrikes() << "/3\n";
                    clearInput();

                    if (player.isBanned()) {
                        cout << "   🚨 [BAN ENGINE] " << player.getName() << " has been permanently BANNED for game engine exploitation!\n";
                        saveSystem(player);
                        break;
                    }
                }
            }

            // Game over checks if all players are out or banned
            if (activeCount == 0) {
                cout << "\n🛑 Session terminated. No active, clean players left on the table.\n";
                break;
            }

            // Phase 2: Generating The Spin
            winningNumber = rand() % MAX_NUMBER + 1;
            cout << "\n\n===============================================\n";
            cout << "🎲 BALL IS ROLLING... WINNING NUMBER IS: [" << winningNumber << "] 🎲\n";
            cout << "===============================================\n\n";

            // Phase 3: Payout Allocation & Sync
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

                saveSystem(player); 

                if (player.getBalance() <= 0) {
                    cout << "💀 BANKRUPT! " << player.getName() << " has run out of funds.\n";
                }
            }

            cout << "\n--> Should the dealer spin another round? (y/n): ";
            cin >> nextRound;
        }

        cout << "\n💰 Session Ended. Database files fully sync'd.\n";
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
