#include <iostream>
#include <string>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <limits>  // For clearing input stream buffer

using namespace std;

class CasinoGame {
private:
    // Game Configuration Constants
    const int MAX_NUMBER = 10;
    const int PAYOUT_MULTIPLIER = 10;

    // Player State Variables
    string playerName;
    int balance;
    int bettingAmount;
    int playerGuess;
    int winningNumber;

    // Private Helper Functions (Internal Logic)
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

    void getBettingAmount() {
        do {
            cout << playerName << ", enter amount to bet: $";
            if (!(cin >> bettingAmount)) {
                cout << "Invalid input! Please enter a valid number.\n";
                clearInputBuffer();
                bettingAmount = -1; // Force loop to continue
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
        // 1. Refresh screen and show layout
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        showRules();
        cout << "\nYour current balance is: $" << balance << "\n\n";

        // 2. Gather verified inputs
        getBettingAmount();
        getPlayerGuess();

        // 3. Game execution logic
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
    }

public:
    // Constructor initializes the random seed
    CasinoGame() {
        srand(static_cast<unsigned int>(time(0)));
        balance = 0;
        bettingAmount = 0;
        playerGuess = 0;
        winningNumber = 0;
    }

    // Public API to bootstrap and start the game execution
    void start() {
        drawLine(60, '=');
        cout << "\n\t\tWELCOME TO THE OOP CASINO\n";
        drawLine(60, '=');

        cout << "\nEnter your name: ";
        getline(cin, playerName);

        do {
            cout << "Enter your starting balance ($): ";
            if (cin >> balance && balance > 0) break;
            cout << "Invalid starting balance. Must be a positive number.\n";
            clearInputBuffer();
        } while (true);

        char choice;
        do {
            playRound();

            if (balance <= 0) {
                cout << "\nBankrupt! You have run out of money. Game Over.\n";
                break;
            }

            cout << "\n--> Do you want to play another round? (y/n): ";
            cin >> choice;

        } while (choice == 'Y' || choice == 'y');

        cout << "\n";
        drawLine(60, '=');
        cout << "Thank you for playing, " << playerName << "! You walk away with $" << balance << ".\n";
        drawLine(60, '=');
    }
};

int main() {
    // Instantiate the class object and launch the game
    CasinoGame game;
    game.start();
    return 0;
}
