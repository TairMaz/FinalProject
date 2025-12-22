#include <bits/stdc++.h>
using namespace std;

struct Job {
    int id;       // מספר עבודה
    int processingTime; // זמן עיבוד (זהה לכל המכונות)
    int weight;   // משקל העבודה
};

// פונקציה שמחשבת זמן סיום לפי הנוסחה: Cj(σ) = Σ(k=1 to j) pk + (m-1)max{p1,…,pj}
int calculateCompletionByFormula(const vector<Job>& jobsPermutation, int jobIndex, int M) {
    int sum = 0;
    int maxP = 0;
    
    // j הוא 1-based בנוסחה, אבל jobIndex הוא 0-based, אז j = jobIndex + 1
    // אז אנחנו סוכמים מ-k=1 עד j, כלומר מ-0 עד jobIndex (כולל)
    for(int k = 0; k <= jobIndex; k++) {
        sum += jobsPermutation[k].processingTime;
        maxP = max(maxP, jobsPermutation[k].processingTime);
    }
    
    return sum + (M - 1) * maxP;
}

// פונקציה שמחשבת את משקל הסיום לפרמוטציה נתונה
int calculateWeightedCompletion(const vector<Job>& jobsPermutation, int M) {
    int N = jobsPermutation.size();
    vector<vector<int>> completion(N, vector<int>(M, 0));

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < M; j++) {
            int timePrevJobSameMachine = (i > 0) ? completion[i-1][j] : 0;
            int timePrevMachineSameJob = (j > 0) ? completion[i][j-1] : 0;
            completion[i][j] = max(timePrevJobSameMachine, timePrevMachineSameJob) + jobsPermutation[i].processingTime;
        }
    }

    int totalWeightedCompletion = 0;
    for(int i = 0; i < N; i++) {
        totalWeightedCompletion += jobsPermutation[i].weight * completion[i][M-1];
    }
    return totalWeightedCompletion;
}

// פונקציה שבודקת אם הנוסחה נותנת את אותה תוצאה כמו החישוב המלא
bool verifyFormula(const vector<Job>& jobsPermutation, int M, bool printDetails = false) {
    int N = jobsPermutation.size();
    vector<vector<int>> completion(N, vector<int>(M, 0));

    // חישוב מלא דרך כל המכונות
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < M; j++) {
            int timePrevJobSameMachine = (i > 0) ? completion[i-1][j] : 0;
            int timePrevMachineSameJob = (j > 0) ? completion[i][j-1] : 0;
            completion[i][j] = max(timePrevJobSameMachine, timePrevMachineSameJob) + jobsPermutation[i].processingTime;
        }
    }

    bool allMatch = true;
    if(printDetails) {
        cerr << "\n=== Formula Verification ===" << endl;
        cerr << "Job | Full Calculation | Formula | Match?" << endl;
        cerr << "----|------------------|---------|--------" << endl;
    }

    for(int i = 0; i < N; i++) {
        int actualCompletion = completion[i][M-1];
        int formulaCompletion = calculateCompletionByFormula(jobsPermutation, i, M);
        bool matches = (actualCompletion == formulaCompletion);
        
        if(!matches) allMatch = false;
        
        if(printDetails) {
            cerr << " " << jobsPermutation[i].id << "  |       " << actualCompletion 
                 << "        |    " << formulaCompletion << "    | " 
                 << (matches ? "YES" : "NO") << endl;
        }
    }

    if(printDetails) {
        if(allMatch) {
            cerr << "\nAll completion times match! Formula is correct." << endl;
        } else {
            cerr << "\nMismatch found between full calculation and formula!" << endl;
        }
    }

    return allMatch;
}

int main() {
    int choice;
    cerr << "Choose input mode: 0 = predefined example, 1 = dynamic input: ";
    cin >> choice;

    vector<Job> jobs;
    int N, M;

    if(choice == 0) {
        // --- דוגמה 1: 3 עבודות, 4 מכונות ---
        // jobs = {
        //     {1, 5, 15},
        //     {2, 3, 6},
        //     {3, 7, 14}
        // };
        // N = jobs.size();
        // M = 4;

        // // --- אם רוצים לבדוק דוגמה אחרת, אפשר להחליף כאן ---
        // // דוגמה 2: 4 עבודות, 3 מכונות
        // jobs = {
        //     {1, 4, 10},
        //     {2, 2, 8},
        //     {3, 6, 12},
        //     {4, 3, 5}
        // };
        // N = jobs.size();
        // M = 3;

        // // דוגמה 3: 4 עבודות, 3 מכונות
        // jobs = {
        //     {1, 12, 8},
        //     {2, 9, 18},
        //     {3, 2, 7},
        //     {4, 17, 20}
        // };
        // N = jobs.size();
        // M = 5;

        jobs = {
            {1, 4, 3},
            {2, 2, 1},
            {3, 6, 2},
            {4, 3, 4}
        };
        N = jobs.size();
        M = 3;


    } else {
        cerr << "Enter number of jobs: ";
        cin >> N;
        cerr << "Enter number of machines: ";
        cin >> M;
        jobs.resize(N);
        for(int i = 0; i < N; i++) {
            jobs[i].id = i+1;
            cerr << "Enter processing time for Job " << i+1 << ": ";
            cin >> jobs[i].processingTime;
            cerr << "Enter weight for Job " << i+1 << ": ";
            cin >> jobs[i].weight;
        }
    }

    // חיפוש הסדר עם המשקל המינימלי
    vector<int> bestPermutation;
    int minWeightedCompletion = INT_MAX;

    vector<int> indices(N);
    iota(indices.begin(), indices.end(), 0); // 0,1,...,N-1

    do {
        vector<Job> currentPermutation;
        for(int idx : indices) currentPermutation.push_back(jobs[idx]);
        int currentWeight = calculateWeightedCompletion(currentPermutation, M);
        if(currentWeight < minWeightedCompletion) {
            minWeightedCompletion = currentWeight;
            bestPermutation = indices;
        }
    } while(next_permutation(indices.begin(), indices.end()));

    cout << "Minimum weighted completion time: " << minWeightedCompletion << endl;
    cout << "Best job order: ";
    for(int idx : bestPermutation) cout << jobs[idx].id << " ";
    cout << endl;

    // בניית הפרמוטציה הטובה ביותר ובדיקת הנוסחה
    vector<Job> bestPermutationJobs;
    for(int idx : bestPermutation) bestPermutationJobs.push_back(jobs[idx]);
    
    cerr << "\n=== Formula Verification for Best Solution (Weight: " << minWeightedCompletion << ") ===" << endl;
    verifyFormula(bestPermutationJobs, M, true);

    // בדיקה נוספת: בודקים את הנוסחה גם על הפרמוטציה הראשונית
    if(choice == 0) {
        cerr << "\n=== Formula Verification for Initial Permutation ===" << endl;
        verifyFormula(jobs, M, true);
    }

    return 0;
}
