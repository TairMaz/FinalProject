#include <bits/stdc++.h>
using namespace std;

struct Job {
    int id;               // מזהה עבודה
    int processingTime;   // p_j
    int weight;           // w_j
};

struct DPResult {
    long long optimalCost;
    vector<int> optimalOrder; // ids של העבודות לפי הסדר
};

DPResult solveSchedulingDP(const vector<Job>& jobs, int M) {
    int n = jobs.size();
    int N = 1 << n;

    const long long INF = LLONG_MAX / 4;

    vector<long long> dp(N, INF);
    vector<int> parent(N, -1);

    // עזרים לחישוב מהיר של C_j
    vector<int> sumP(N, 0);
    vector<int> maxP(N, 0);

    // קדם־חישוב sumP ו־maxP לכל קבוצה
    for (int S = 1; S < N; ++S) {
        int j = __builtin_ctz(S);      // הביט הדלוק הראשון
        int prev = S & ~(1 << j);

        sumP[S] = sumP[prev] + jobs[j].processingTime;
        maxP[S] = max(maxP[prev], jobs[j].processingTime);
    }

    dp[0] = 0;

    // תכנות דינמי
    for (int S = 0; S < N; ++S) {
        if (dp[S] == INF) continue;

        for (int j = 0; j < n; ++j) {
            if (S & (1 << j)) continue;

            int S2 = S | (1 << j);

            // C_j לפי הנוסחה
            long long Cj =
                sumP[S2] + (long long)(M - 1) * maxP[S2];

            long long newCost =
                dp[S] + (long long)jobs[j].weight * Cj;

            if (newCost < dp[S2]) {
                dp[S2] = newCost;
                parent[S2] = j;
            }
        }
    }

    // שחזור הפרמוטציה
    vector<int> order;
    int S = N - 1;

    while (S) {
        int j = parent[S];
        order.push_back(jobs[j].id);
        S ^= (1 << j);
    }

    reverse(order.begin(), order.end());

    return { dp[N - 1], order };
}

struct NaiveResult {
    int optimalCost;
    vector<int> optimalOrder; // ids
};

NaiveResult solveNaive(const vector<Job>& jobs, int M) {
    vector<Job> perm = jobs;

    // חשוב: להתחיל מסידור קנוני
    sort(perm.begin(), perm.end(),
         [](const Job& a, const Job& b) {
             return a.id < b.id;
         });

    int bestCost = INT_MAX;
    vector<int> bestOrder;

    do {
        int N = perm.size();
        vector<vector<int>> completion(N, vector<int>(M, 0));

        // סימולציית Flow Shop
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                int timePrevJobSameMachine =
                    (i > 0) ? completion[i - 1][j] : 0;
                int timePrevMachineSameJob =
                    (j > 0) ? completion[i][j - 1] : 0;

                completion[i][j] =
                    max(timePrevJobSameMachine, timePrevMachineSameJob)
                    + perm[i].processingTime;
            }
        }

        int totalWeightedCompletion = 0;
        for (int i = 0; i < N; i++) {
            totalWeightedCompletion +=
                perm[i].weight * completion[i][M - 1];
        }

        // בדיקת שיפור
        if (totalWeightedCompletion < bestCost) {
            bestCost = totalWeightedCompletion;
            bestOrder.clear();
            for (const auto& job : perm)
                bestOrder.push_back(job.id);
        }

    } while (next_permutation(perm.begin(), perm.end(),
                              [](const Job& a, const Job& b) {
                                  return a.id < b.id;
                              }));

    return { bestCost, bestOrder };
}

int main() {
    vector<Job> jobs = {
        {1, 4, 3},
        {2, 2, 1},
        {3, 6, 2},
        {4, 3, 4}
    };

    int M = 3;

    NaiveResult naiveRes = solveNaive(jobs, M);
    DPResult dpRes = solveSchedulingDP(jobs, M);

    cout << "Naive cost: " << naiveRes.optimalCost << "\n";
    cout << "Naive order: ";
    for (int id : naiveRes.optimalOrder)
        cout << id << " ";
    cout << "\n\n";

    cout << "DP cost: " << dpRes.optimalCost << "\n";
    cout << "DP order: ";
    for (int id : dpRes.optimalOrder)
        cout << id << " ";
    cout << "\n";

    return 0;
}
