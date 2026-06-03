#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <ctime>   
#include <limits>  
#include <fstream> 

using namespace std;

// ============================================================================
// PLAYER CLASS (Encapsulates individual player data and state)
// ============================================================================
class Player {
private:
    string name;
    int balance;
    int currentBet;
    int currentGuess;
    bool active;

public:
    Player(string p_name, int p_balance) 
        : name(p_name), balance(p_balance), currentBet(0), currentGuess(0), active(true) {}

    // Getters
    string getName() const { return name; }
    int getBalance() const { return balance; }
    int getBet() const { return currentBet; }
    int getGuess() const { return currentGuess; }
    bool isActive() const { return active; }

    // Setters & State Modifiers
    void setInactive() { active = false; }
    
    bool placeBet(int amt) {
        if (amt > 0 && amt <= balance) {
            currentBet = amt;
            return true;
        }
        return false;
    }

    bool makeGuess(int g, int maxNum) {
        if (g >= 1 && g <= maxNum) {
            currentGuess = g;
            return true;
        }
        return false;
    }

    void updateBalance(bool won, int payoutMultiplier) {
        if (won) {
            balance += (currentBet * payoutMultiplier);
        } else {
            balance -= currentBet;
        }
        if (balance <= 0) {
            active = false; // Automatically deactivate if the player goes bankrupt
        }
    }
};

// ============================================================================
// CASINO ENGINE CLASS (Handles game orchestration, multiplayer turns, and I/O)
// ============================================================================
class AdvancedCasino {
private:
    const int MAX_NUMBER = 10;
    const int PAYOUT_MULTIPLIER = 10;
    const string SAVE_EXT = "_save.txt";

    vector<Player> tablePlayers; // Dynamic roster holding active players
    int winningNumber;

    // Helper to clear invalid stream input and prevent infinite loops
    void clearInput() const {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Save System (Persistence File Output)
    void saveSystem(const Player& p) {
        ofstream file(p.getName() + SAVE_EXT);
        if (file.is_open()) {
            file << p.getBalance();
            file.close();
        }
    }

    // Load System (Persistence File Input)
    int loadSystem(const string& name) {
        ifstream file(name + SAVE_EXT);
        if (file.is_open()) {
            int bal;
            file >> bal;
            file.close();
            return bal;
        }
        return -1; // Flag indicating no previous save profile exists
    }

public:
    AdvancedCasino() : winningNumber(0) {
        srand(static_cast<unsigned int>(time(0))); // Seed randomizer using epoch time
    }

    // Registers and setups multiple players dynamically
    void registerPlayers() {
        int count;
        while (true) {
            cout << "🎰 How many players want to join the table? (1-5): ";
            if (cin >> count && count >= 1 && count <= 5) break;
            cout << "❌ Invalid choice! Please enter a valid integer between 1 and 5.\n";
            clearInput();
        }
        clearInput();

        for (int i = 0; i < count; ++i) {
            string name;
            cout << "👤 Enter Name for Player " << i + 1 << ": ";
            getline(cin, name);

            int savedBal = loadSystem(name);
            if (savedBal > 0) {
                cout << "   [Profile Found] Welcome back, " << name << "! Loaded Balance: $" << savedBal << "\n";
                tablePlayers.push_back(Player(name, savedBal));
            } else {
                int startBal;
                while (true) {
                    cout << "   " << name << ", enter your starting chip balance ($): ";
                    if (cin >> startBal && startBal > 0) break;
                    cout << "   ❌ Invalid amount! Please enter a positive cash value.\n";
                    clearInput();
                }
                clearInput();
                Player newPlayer(name, startBal);
                tablePlayers.push_back(newPlayer);
                saveSystem(newPlayer);
            }
        }
    }

    // Core execution loop for the game sessions
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

            // Phase 1: Processing inputs for all active players sequentially
            for (auto& player : tablePlayers) {
                if (!player.isActive()) continue;
                
                activeCount++;
                cout << "\n👉 TURN: " << player.getName() << " | Current Chips: $" << player.getBalance() << "\n";

                // Bet Segment
                int bet;
                while (true) {
                    cout << "   Enter bet amount (Or press '0' to cash out and leave table): $";
                    if (cin >> bet) {
                        if (bet == 0) {
                            player.setInactive();
                            cout << "   👋 " << player.getName() << " has cashed out and left the session.\n";
                            break;
                        }
                        if (player.placeBet(bet)) break;
                    }
                    cout << "   ❌ Invalid Bet! Amount must be greater than 0 and within your available chips.\n";
                    clearInput();
                }

                if (!player.isActive()) continue; // Skip to next player if current player left the table

                // Guess Segment
                int guess;
                while (true) {
                    cout << "   Guess the lucky
