#include <climits>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

// ה-struct המעודכן עם השמות המבוקשים
struct Job {
    int id;
    double processingTime; // p
    double weight;         // w
    double ratio;          // w/p
};

struct NaiveResult {
    double optimalCost;
    vector<int> optimalOrder;
};

// פונקציה לחישוב עלות מלאה לפי נוסחת הקירוב (MCI) - O(n)
double calculateMCI_Cost(const vector<Job>& schedule, int m) {
    double total = 0, sumP = 0, maxP = 0;
    for (const auto& j : schedule) {
        sumP += j.processingTime;
        maxP = max(maxP, j.processingTime);
        total += j.weight * (sumP + (m - 1) * maxP);
    }
    return total;
}

// אלגוריתם WSPT-MCI בסיבוכיות O(n^2)
void solveWSPT_MCI_PureN2(vector<Job> jobs, int m) {
    int n = jobs.size();
    
    // שלב 1: מיון WSPT
    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.ratio > b.ratio;
    });

    vector<Job> currentSchedule;

    for (int k = 0; k < n; ++k) {
        Job jNew = jobs[k];
        if (currentSchedule.empty()) {
            currentSchedule.push_back(jNew);
            continue;
        }

        int sz = currentSchedule.size();
        vector<double> prefSumP(sz), prefMaxP(sz), suffSumW(sz);
        
        // הכנת מערכי עזר ב-O(k)
        double sP = 0, mP = 0;
        for (int i = 0; i < sz; ++i) {
            sP += currentSchedule[i].processingTime;
            mP = max(mP, currentSchedule[i].processingTime);
            prefSumP[i] = sP;
            prefMaxP[i] = mP;
        }
        double sW = 0;
        for (int i = sz - 1; i >= 0; --i) {
            sW += currentSchedule[i].weight;
            suffSumW[i] = sW;
        }

        int bestPos = -1;
        double minScore = -1;

        // חיפוש מיקום אופטימלי ב-O(1) לאיטרציה
        for (int i = 0; i <= sz; ++i) {
            double pBefore = (i == 0) ? 0 : prefSumP[i - 1];
            double mBefore = (i == 0) ? 0 : prefMaxP[i - 1];

            // עלות העבודה החדשה במיקום i
            double mySumP = pBefore + jNew.processingTime;
            double myMaxP = max(mBefore, jNew.processingTime);
            double myCost = jNew.weight * (mySumP + (m - 1) * myMaxP);

            // תוספת לעבודות שאחרי בגלל הכנסת החדשה
            double afterCostGain = 0;
            if (i < sz) {
                afterCostGain += suffSumW[i] * jNew.processingTime;
                if (jNew.processingTime > prefMaxP[sz - 1]) {
                    afterCostGain += (m - 1) * (jNew.processingTime - prefMaxP[sz - 1]) * suffSumW[i];
                }
            }

            double estimatedTotal = myCost + afterCostGain; 

            if (bestPos == -1 || estimatedTotal < minScore) {
                minScore = estimatedTotal;
                bestPos = i;
            }
        }
        currentSchedule.insert(currentSchedule.begin() + bestPos, jNew);
    }

    cout << "--- WSPT-MCI Algorithm (O(n^2)) ---" << endl;
    cout << "Order: ";
    for (const auto& j : currentSchedule) cout << j.id << " ";
    cout << "\nCost: " << calculateMCI_Cost(currentSchedule, m) << endl << endl;
}

// אלגוריתם נאיבי (Permutations) - סימולציית Flow Shop
NaiveResult solveNaive(const vector<Job>& jobs, int M) {
    vector<Job> perm = jobs;
    sort(perm.begin(), perm.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    });

    double bestCost = 1e18; // ערך גבוה מאוד להתחלה
    vector<int> bestOrder;

    do {
        int N = perm.size();
        // טבלת זמני סיום: completion[עבודה][מכונה]
        vector<vector<double>> completion(N, vector<double>(M, 0));

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                double timePrevJobSameMachine = (i > 0) ? completion[i - 1][j] : 0;
                double timePrevMachineSameJob = (j > 0) ? completion[i][j - 1] : 0;

                completion[i][j] = max(timePrevJobSameMachine, timePrevMachineSameJob) 
                                   + perm[i].processingTime;
            }
        }

        double totalWeightedCompletion = 0;
        for (int i = 0; i < N; i++) {
            totalWeightedCompletion += perm[i].weight * completion[i][M - 1];
        }

        if (totalWeightedCompletion < bestCost) {
            bestCost = totalWeightedCompletion;
            bestOrder.clear();
            for (const auto& job : perm) bestOrder.push_back(job.id);
        }

    } while (next_permutation(perm.begin(), perm.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    }));

    return { bestCost, bestOrder };
}

int main() {
    int M = 3;
    // הגדרה עם השמות החדשים
    vector<Job> jobs = {
        {1, 6, 12}, 
        {2, 5, 8}, 
        {3, 9, 15}, 
        {4, 12, 5},
        {5, 15, 12},
        {6, 18, 23}

    };

    // חישוב יחס w/p
    for (auto& j : jobs) {
        j.ratio = j.weight / j.processingTime;
    }

    // הרצת האלגוריתם המיועל
    solveWSPT_MCI_PureN2(jobs, M);

    // הרצת האלגוריתם הנאיבי
    NaiveResult naiveRes = solveNaive(jobs, M);

    cout << "--- Naive Algorithm (O(n!)) ---" << endl;
    cout << "Optimal Order: ";
    for (int id : naiveRes.optimalOrder) cout << id << " ";
    cout << endl;
    cout << "Optimal Cost: " << naiveRes.optimalCost << endl;


    return 0;
}