#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <memory>
#include <random>
#include <chrono>

using namespace std;

struct Job {
    int id;
    double processingTime;
    double weight;
    double ratio;
    double rejectionCost;
};

struct ScheduleSummary {
    vector<int> scheduledJobs;
    vector<int> rejectedJobs;
    double cost = 0;
    double rejectionCost = 0;
};

struct NaiveResult {
    double cost;
    vector<int> order;
    vector<int> rejected;
    double rejectionCost;
};

struct InsertResult {
    shared_ptr<const vector<Job>> schedule;
    double cost = numeric_limits<double>::infinity();
};

namespace {
    constexpr double COST_EPS = 1e-9;
    constexpr double INF = numeric_limits<double>::infinity();
}

//  FLOW SHOP COST (ל-NAIVE)
double calculateFlowshopCost(const vector<Job>& schedule, int m) {

    int n = static_cast<int>(schedule.size());
    if (n == 0) return 0;

    vector<vector<double>> completion(n, vector<double>(m, 0));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            double left = (j > 0) ? completion[i][j - 1] : 0;
            double up   = (i > 0) ? completion[i - 1][j] : 0;

            completion[i][j] = max(left, up) + schedule[i].processingTime;
        }
    }

    double total = 0;
    for (int i = 0; i < n; i++) {
        total += schedule[i].weight * completion[i][m - 1];
    }

    return total;
}

// INSERTION 
InsertResult insertJobWithBestPosition(const shared_ptr<const vector<Job>>& schedulePtr,
                                      const Job& job,
                                      int m,
                                      double baseCost) {

    
    const vector<Job>* schedule = schedulePtr ? schedulePtr.get() : nullptr;
    const size_t n = schedule ? schedule->size() : 0;

    // מקרה בסיס: אין עבודות עדיין
    if (n == 0) {
        auto newSchedule = make_shared<vector<Job>>();
        newSchedule->push_back(job);

        // כאן מחשבים ישירות את MCI של עבודה אחת
        double sumP = job.processingTime;
        double maxP = job.processingTime;
        double cost = baseCost + job.weight * (sumP + (m - 1) * maxP);

        return {newSchedule, cost};
    }

    const auto& seq = *schedule;

    // ===== prefix arrays =====
    // מאפשרים לחשב את השינוי ב-MCI בזמן O(1) לכל מיקום
    vector<double> prefixSum(n + 1, 0.0);        // סכום p
    vector<double> prefixMax(n + 1, 0.0);        // מקסימום p
    vector<double> prefixWeight(n + 1, 0.0);     // סכום w
    vector<double> prefixWeightMax(n + 1, 0.0);  // סכום w * max

    for (size_t i = 0; i < n; ++i) {
        prefixSum[i + 1] = prefixSum[i] + seq[i].processingTime;
        prefixMax[i + 1] = max(prefixMax[i], seq[i].processingTime);
        prefixWeight[i + 1] = prefixWeight[i] + seq[i].weight;
        prefixWeightMax[i + 1] = prefixWeightMax[i] + seq[i].weight * prefixMax[i + 1];
    }

    double bestCost = INF;
    size_t bestPos = 0;

    // עוזר למצוא מאיפה max משתנה
    size_t splitPrefix = lower_bound(prefixMax.begin() + 1, prefixMax.end(), job.processingTime) - prefixMax.begin();

    for (size_t pos = 0; pos <= n; ++pos) {

        // תיקון אינדקסים כדי לשמור על חלוקה נכונה של prefix
        if (splitPrefix < pos + 1) {
            splitPrefix = pos + 1;
        }
        while (splitPrefix <= n && prefixMax[splitPrefix] < job.processingTime) {
            ++splitPrefix;
        }

        size_t splitIdx = (splitPrefix <= n) ? splitPrefix - 1 : n;
        if (splitIdx < pos) {
            splitIdx = pos;
        }

        // ===== חלק 1: התרומה של העבודה החדשה =====
        double sumBefore = prefixSum[pos];
        double maxBefore = prefixMax[pos];
        double pStar = job.processingTime;
        double newMax = max(maxBefore, pStar);

        double candidate = baseCost +
            job.weight * (sumBefore + pStar + (m - 1) * newMax);

        // ===== חלק 2: השפעה על עבודות אחרי ההכנסה =====
        if (splitIdx > pos) {
            double weightLess = prefixWeight[splitIdx] - prefixWeight[pos];
            if (weightLess > 0) {
                double sumWeightMax = prefixWeightMax[splitIdx] - prefixWeightMax[pos];

                // עדכון מדויק של MCI במקום חישוב מחדש
                candidate += (pStar * m) * weightLess - (m - 1) * sumWeightMax;
            }
        }

        // עבודות אחרי splitIdx מושפעות רק בזמן עיבוד
        double suffixWeight = prefixWeight[n] - prefixWeight[splitIdx];
        if (suffixWeight > 0) {
            candidate += suffixWeight * pStar;
        }

        // שומר את המיקום הכי טוב
        if (candidate + COST_EPS < bestCost) {
            bestCost = candidate;
            bestPos = pos;
        }
    }

    // בניית הסידור בפועל 
    auto bestSchedule = make_shared<vector<Job>>();
    bestSchedule->reserve(n + 1);

    auto splitIt = seq.begin() + static_cast<typename vector<Job>::difference_type>(bestPos);

    bestSchedule->insert(bestSchedule->end(), seq.begin(), splitIt);
    bestSchedule->push_back(job);
    bestSchedule->insert(bestSchedule->end(), splitIt, seq.end());

    return {bestSchedule, bestCost};
}

/*
 * פונקציה זו מממשת הכנסת עבודה חדשה למיקום אופטימלי בתוך סידור קיים
 * בהתאם לפונקציית המטרה MCI (Minimum Completion Time approximation).
 *
 * במקום לחשב מחדש את כל עלות ה-MCI עבור כל מיקום אפשרי (מה שהיה עולה O(n^2)),
 * אנו משתמשים בנוסחה אינקרמנטלית (incremental), שמחשבת רק את השינוי בעלות.
 *
 * הרעיון המרכזי:
 * פונקציית MCI מוגדרת כ:
 *     sum_j w_j * (sum_{k<=j} p_k + (m-1) * max_{k<=j} p_k)
 *
 * כאשר מוסיפים עבודה חדשה, אין צורך לחשב מחדש את כל הסכום.
 * במקום זאת:
 *     MCI חדש = MCI ישן (baseCost) + שינוי (delta)
 *
 * השינוי (delta) מורכב משני חלקים:
 * 1. התרומה של העבודה החדשה עצמה.
 * 2. ההשפעה של העבודה החדשה על זמני הסיום של העבודות שאחריה.
 *
 * כדי לחשב זאת ביעילות, משתמשים במערכי prefix:
 * - prefixSum: סכום מצטבר של זמני עיבוד
 * - prefixMax: המקסימום עד כל נקודה
 * - prefixWeight: סכום משקלים מצטבר
 * - prefixWeightMax: סכום משוקלל של המקסימום
 *
 * בעזרת מערכים אלו ניתן לחשב את עלות כל מיקום ב-O(1),
 * ולכן סך כל הכנסת עבודה מתבצע בזמן O(n) במקום O(n^2).
 *
 * חשוב:
 * המימוש שקול מתמטית לנוסחת MCI המקורית — לא מדובר בקירוב או שינוי האלגוריתם,
 * אלא רק באופטימיזציה של אופן החישוב.
 */
////O(n^2⋅U) זמן ריצה
ScheduleSummary solveWsptDP(vector<Job> jobs, int m, double U) {

    // מיון לפי WSPT 
    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        if (a.ratio == b.ratio) return a.id < b.id;
        return a.ratio > b.ratio;
    });

    const int budgetLimit = static_cast<int>(U);
    const int n = static_cast<int>(jobs.size());

    struct Cell {
        // reachable[b]
        // מציין האם ניתן להגיע לסכום דחיות b, באמצעות החלטות keep/reject, עד כה
        // מבלי לעבור את התקציב הכולל U.
        bool reachable = false;
        double cost = INF; // שומר MCI עד כה
        shared_ptr<const vector<Job>> schedule;// הסידור עצמו (ללא העתקות כבדות)
    };

    // משתמשים רק בשתי שכבות DP (prev/curr) כדי לחסוך זיכרון
    vector<Cell> prev(budgetLimit + 1);
    vector<Cell> curr(budgetLimit + 1);

    // לשחזור הפתרון
    vector<vector<int>> prevBudget(n + 1, vector<int>(budgetLimit + 1, -1));
    vector<vector<bool>> tookJob(n + 1, vector<bool>(budgetLimit + 1, false));

    // מצב התחלה
    prev[0].reachable = true;
    prev[0].cost = 0.0;
    prev[0].schedule = make_shared<vector<Job>>();

    for (int i = 1; i <= n; ++i) {
        const Job& job = jobs[i - 1];
        int rejectCost = static_cast<int>(job.rejectionCost);

        // איפוס השורה הנוכחית
        for (auto& cell : curr) {
            cell.reachable = false;
            cell.cost = INF;
            cell.schedule.reset();
        }

        for (int b = 0; b <= budgetLimit; ++b) {
            if (!prev[b].reachable) continue;

            // ===== KEEP =====
            // מוסיפים עבודה תוך שימוש ב-MCI אינקרמנטלי
            InsertResult keepRes = insertJobWithBestPosition(prev[b].schedule, job, m, prev[b].cost);

            // שומרים רק את הפתרון הכי טוב לכל (i,b)
            if (!curr[b].reachable || keepRes.cost + COST_EPS < curr[b].cost) {
                curr[b].reachable = true;
                curr[b].cost = keepRes.cost;
                curr[b].schedule = keepRes.schedule;

                prevBudget[i][b] = b;
                tookJob[i][b] = true;
            }

            // ===== REJECT =====
            // לא מכניסים את העבודה — רק מעלים תקציב
            int nb = b + rejectCost;
            if (nb > budgetLimit) continue;

            // כאן העלות לא משתנה (כי לא שינינו את הסידור)
            if (!curr[nb].reachable || prev[b].cost + COST_EPS < curr[nb].cost) {
                curr[nb].reachable = true;
                curr[nb].cost = prev[b].cost;
                curr[nb].schedule = prev[b].schedule;

                prevBudget[i][nb] = b;
                tookJob[i][nb] = false;
            }
        }

        // מעבר לשכבה הבאה (DP מתקדם עבודה אחת קדימה)
        prev.swap(curr);
    }

    // בחירת הפתרון הטוב ביותר מכל התקציבים האפשריים
    double bestCost = INF;
    int bestBudget = -1;

    for (int b = 0; b <= budgetLimit; ++b) {
        if (!prev[b].reachable) continue;

        if (prev[b].cost + COST_EPS < bestCost) {
            bestCost = prev[b].cost;
            bestBudget = b;
        }
    }

    if (bestBudget == -1) return {};

    //  שחזור הפתרון 
    // חוזרים אחורה ומזהים אילו עבודות נדחו
    vector<int> rejected;
    int b = bestBudget;

    for (int i = n; i >= 1; --i) {
        if (!tookJob[i][b]) {
            rejected.push_back(jobs[i - 1].id);
        }

        int prevB = prevBudget[i][b];
        b = prevB < 0 ? 0 : prevB;
    }

    sort(rejected.begin(), rejected.end());

    // בניית הפלט
    ScheduleSummary res;

    if (const auto& schedule = prev[bestBudget].schedule) {
        for (const auto& job : *schedule) {
            res.scheduledJobs.push_back(job.id);
        }
    }

    res.rejectedJobs = move(rejected);
    res.cost = prev[bestBudget].cost; // זה כבר ערך MCI הסופי
    res.rejectionCost = static_cast<double>(bestBudget);

    return res;
}

//  NAIVE (עם FLOWSHOP)
NaiveResult solveNaive(const vector<Job>& jobs, int m, double U) {

    vector<Job> perm = jobs;
    sort(perm.begin(), perm.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    });

    double bestCost = 1e18;
    vector<int> bestOrder;
    vector<int> bestRejected;
    double bestRejectionCost = 0;

    do {
        int N = static_cast<int>(perm.size());
        int subsets = 1 << N;

        for (int mask = 0; mask < subsets; mask++) {

            double rejectionCost = 0;
            vector<Job> scheduled;
            vector<int> rejected;

            for (int i = 0; i < N; i++) {
                if (mask & (1 << i)) {
                    rejectionCost += perm[i].rejectionCost;
                    rejected.push_back(perm[i].id);
                } else {
                    scheduled.push_back(perm[i]);
                }
            }

            if (rejectionCost > U) continue;

            double cost = calculateFlowshopCost(scheduled, m);

            if (cost < bestCost) {
                bestCost = cost;

                bestOrder.clear();
                for (auto& j : scheduled)
                    bestOrder.push_back(j.id);

                bestRejected = rejected;
                bestRejectionCost = rejectionCost;
            }
        }

    } while (next_permutation(perm.begin(), perm.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    }));

    return { bestCost, bestOrder, bestRejected, bestRejectionCost };
}

// יוצר n עבודות רנדומליות בטווחים מוגבלים
vector<Job> generateRandomJobs(int n, int maxNum) {
    vector<Job> jobs;

    mt19937 gen(chrono::high_resolution_clock::now().time_since_epoch().count());

    uniform_int_distribution<> pDist(1, maxNum);  // processingTime
    uniform_int_distribution<> wDist(1, maxNum);  // weight
    uniform_int_distribution<> rDist(1, maxNum);  // rejectionCost

    for (int i = 0; i < n; i++) {
        Job j;
        j.id = i + 1;
        j.processingTime = pDist(gen);
        j.weight = wDist(gen);
        j.rejectionCost = rDist(gen);
        j.ratio = j.weight / j.processingTime;

        jobs.push_back(j);
    }

    return jobs;
}

int main() {

    int M = 3;
    double U = 10;
    int numJobs = 5;
    int maxNum = 20;
    vector<Job> jobs = generateRandomJobs(numJobs, maxNum);

    // vector<Job> jobs = {
    //     {1, 4, 10, 0, 4}, 
    //     {2, 2, 8, 0, 3}, 
    //     {3, 6, 12, 0, 6}, 
    //     {4, 3, 5, 0, 5},
    //     {5, 12, 12, 0, 8}
    // };


    cout << "Jobs:\n";
    for (auto& j : jobs) {
        cout << "Job " << j.id
            << " p=" << j.processingTime
            << " w=" << j.weight
            << " r=" << j.rejectionCost << endl;
    }
    cout << endl;

    auto dp = solveWsptDP(jobs, M, U);

    cout << "--- DP (WSPT + MCI) ---\n";
    cout << "Scheduled: ";
    for (int x : dp.scheduledJobs) cout << x << " ";
    cout << "\nRejected: ";
    for (int x : dp.rejectedJobs) cout << x << " ";
    cout << "\nCost: " << dp.cost;
    cout << "\nRejection Cost: " << dp.rejectionCost << "\n\n";

    auto naive = solveNaive(jobs, M, U);

    cout << "--- Naive (Flowshop) ---\n";
    cout << "Scheduled: ";
    for (int x : naive.order) cout << x << " ";
    cout << "\nRejected: ";
    for (int x : naive.rejected) cout << x << " ";
    cout << "\nCost: " << naive.cost;
    cout << "\nRejection Cost: " << naive.rejectionCost << endl;

    return 0;
}
