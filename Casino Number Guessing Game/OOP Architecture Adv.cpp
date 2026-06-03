#include <iostream>
#include <string>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <limits>  // For clearing input stream buffer
#include <fstream> // <-- NEW: File handling ke liye zaroori hai

using namespace std;

class CasinoGame {
private:
    const int MAX_NUMBER = 10;
    const int PAYOUT_MULTIPLIER = 10;
    const string SAVE_FILE_EXTENSION = "_save.txt"; // Har player ki alag file banegi

    string playerName;
    int balance;
    int bettingAmount;
    int playerGuess;
    int winningNumber;

    void drawLine(int n, char symbol) const {
        for (int i = 0; i < n; i++) cout << symbol;
        cout << "\n";
    }

    void showRules() const {
        drawLine(60, '-');
        cout << "\t\t\tRULES OF THE GAME\n";
        drawLine(60, '-');
        cout << "\t1. Choose any number between 1 to " << MAX_NUMBER << "\n";
        cout << "\t2. Win gets you " << PAYOUT_MULTIPLIER << "x your original bet amount.\n";
        cout << "\t3. Wrong guess? You lose your entire bet amount.\n";
        drawLine(60, '-');
    }

    void clearInputBuffer() const {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // --- NEW: Game State ko Save karne ka function ---
    void saveGameState() {
        string fileName = playerName + SAVE_FILE_EXTENSION;
        ofstream outFile(fileName); // File ko write mode me kholna

        if (outFile.is_open()) {
            outFile << balance; // File me sirf balance save kar rahe hain
            outFile.close();
            cout << "[System: Game progress saved successfully.]\n";
        } else {
            cout << "[System Error: Progress save nahi ho paya!]\n";
        }
    }

    // --- NEW: Game State ko Load karne ka function ---
    bool loadGameState() {
        string fileName = playerName + SAVE_FILE_EXTENSION;
        ifstream inFile(fileName); // File ko read mode me kholna

        if (inFile.is_open()) {
            inFile >> balance; // File se balance read karna
            inFile.close();
            return true; // Load successful
        }
        return false; // Koi purani save file nahi mili
    }

    void getBettingAmount() {
        do {
            cout << playerName << ", enter amount to bet: $";
            if (!(cin >> bettingAmount)) {
                cout << "Invalid input! Please enter a valid number.\n";
                clearInputBuffer();
                bettingAmount = -1;
                continue;
            }
            if (bettingAmount <= 0) {
                cout << "Betting amount must be greater than $0!\n";
            } else if (bettingAmount > balance) {
                cout << "You cannot bet more than your current balance ($" << balance << ")!\n";
            }
        } while (bettingAmount <= 0 || bettingAmount > balance);
    }

    void getPlayerGuess() {
        do {
            cout << "Guess the winning number (1 - " << MAX_NUMBER << "): ";
            if (!(cin >> playerGuess)) {
                cout << "Invalid input! Please enter an integer.\n";
                clearInputBuffer();
                playerGuess = -1;
                continue;
            }
            if (playerGuess < 1 || playerGuess > MAX_NUMBER) {
                cout << "Number must be strictly between 1 and " << MAX_NUMBER << "!\n";
            }
        } while (playerGuess < 1 || playerGuess > MAX_NUMBER);
    }

    void playRound() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        showRules();
        cout << "\nYour current balance is: $" << balance << "\n\n";

        getBettingAmount();
        getPlayerGuess();

        winningNumber = rand() % MAX_NUMBER + 1;

        if (winningNumber == playerGuess) {
            int winnings = bettingAmount * PAYOUT_MULTIPLIER;
            cout << "\n\n🎉 JACKPOT! You guessed correctly!";
            cout << "\nYou won: $" << winnings << "\n";
            balance += winnings;
        } else {
            cout << "\n\n❌ House wins! Better luck next time.";
            cout << "\nYou lost: $" << bettingAmount << "\n";
            balance -= bettingAmount;
        }

        cout << "The winning number was: " << winningNumber << "\n";
        cout << playerName << ", your updated balance is: $" << balance << "\n";
        
        // Har round ke baad auto-save ho jayega
        saveGameState(); 
    }

public:
    CasinoGame() {
        srand(static_cast<unsigned int>(time(0)));
        balance = 0;
        bettingAmount = 0;
        playerGuess = 0;
        winningNumber = 0;
    }

    void start() {
        drawLine(60, '=');
        cout << "\n\t\tWELCOME TO THE OOP CASINO WITH SAVES\n";
        drawLine(60, '=');

        cout << "\nEnter your name: ";
        getline(cin, playerName);

        // --- NEW: Load Logic check ---
        if (loadGameState()) {
            cout << "\nWelcome back, " << playerName << "! Purana data mil gaya hai.\n";
            cout << "Aapka loaded balance hai: $" << balance << "\n\n";
        } else {
            cout << "\nNaye khiladi lagte ho! ";
            do {
                cout << "Enter your starting balance ($): ";
                if (cin >> balance && balance > 0) break;
                cout << "Invalid starting balance. Must be a positive number.\n";
                clearInputBuffer();
            } while (true);
            saveGameState(); // Shuruat me hi ek file bana di
        }

        char choice;
        do {
            playRound();

            if (balance <= 0) {
                cout << "\nBankrupt! You have run out of money. Game Over.\n";
                saveGameState(); // 0 balance save ho jayega taaki agli baar se naya start ho
                break;
            }

            cout << "\n--> Do you want to play another round? (y/n): ";
            cin >> choice;

        } while (choice == 'Y' || choice == 'y');

        cout << "\n";
        drawLine(60, '=');
        cout << "Thank you for playing, " << playerName << "! Progress saved. Final balance: $" << balance << ".\n";
        drawLine(60, '=');
    }
};

int main() {
    CasinoGame game;
    game.start();
    return 0;
}
