#include <iostream>
#include <vector>
using namespace std;

// פונקציה לפתרון בעיית התרמיל (0/1 Knapsack)
int knapsack(int maxWeight, const vector<int>& weights, const vector<int>& values) {
    int n = weights.size();

    // טבלת DP בגודל (n+1) על (maxWeight+1)
    vector<vector<int>> dp(n + 1, vector<int>(maxWeight + 1, 0));

    // מילוי הטבלה
    for (int i = 1; i <= n; i++) {
        for (int w = 1; w <= maxWeight; w++) {

            // אם אפשר לקחת את החפץ
            if (weights[i - 1] <= w) {
                dp[i][w] = max(
                    values[i - 1] + dp[i - 1][w - weights[i - 1]], // לקחת
                    dp[i - 1][w]                                   // לא לקחת
                );
            } else {
                // אי אפשר לקחת אותו כי הוא כבד מדי
                dp[i][w] = dp[i - 1][w];
            }
        }
    }

    return dp[n][maxWeight]; // התוצאה הסופית
}

int main() {

    // // example 1: 
    // int maxWeight = 10;
    // vector<int> weights = {5, 4, 6};
    // vector<int> values  = {10, 40, 30};
    // // expected: 70

    // // example 2:
    // int maxWeight = 8;
    // vector<int> weights = {2, 3, 4, 5};
    // vector<int> values  = {3, 4, 5, 8};
    // // expected: 12

    // // example 3:
    // int maxWeight = 15;
    // vector<int> weights = {1, 4, 5, 7, 11};
    // vector<int> values  = {1, 4, 7, 10, 15};
    // // expected: 19

    // // example 4:
    // int maxWeight = 20;
    // vector<int> weights = {3, 5, 7, 9, 4, 6};
    // vector<int> values  = {4, 7, 10, 13, 6, 8};
    // // expected: 29

    // // example 5:
    // int maxWeight = 50;
    // vector<int> weights = {10, 20, 30, 40, 5};
    // vector<int> values  = {60, 100, 120, 240, 30};
    // // expected: 300

    // // example 6:
    // int maxWeight = 35;
    // vector<int> weights = {5, 10, 12, 13, 15, 7, 9};
    // vector<int> values  = {10, 20, 24, 25, 30, 13, 18};
    // // expected: 69

    // // example 7:
    // int maxWeight = 15;
    // vector<int> weights = {1, 2, 3, 4, 5, 6};
    // vector<int> values  = {2, 4, 4, 5, 7, 9};
    // // expected: 22

    // example 8:
    int maxWeight = 100;
    vector<int> weights = {10, 20, 30, 40, 50, 1, 5, 6, 7};
    vector<int> values  = {60, 90, 120, 200, 240, 1, 8, 9, 10};
    // expected: 







    int result = knapsack(maxWeight, weights, values);

    cout << "the maximal value: " << result << endl;

    return 0;
}
