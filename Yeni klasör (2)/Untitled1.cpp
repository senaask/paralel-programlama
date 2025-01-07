#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <conio.h>
#include <clocale>

#define NUM_TASKS 16
#define NUM_ITERATIONS 200
#define PROGRESS_BAR_HEIGHT 20

// Görevlerin tamamlanma durumu ve sürelerini tutan diziler
int taskCompletionStatus[NUM_TASKS], taskDuration[NUM_TASKS];
unsigned long long taskStartTime[NUM_TASKS], taskEndTime[NUM_TASKS];
volatile int terminateExecution = 0;
volatile int taskFinished[NUM_TASKS] = {0};

// Konsol imlecini belirtilen (x, y) koordinatlarýna taþýyan fonksiyon
void setCursorPosition(int x, int y) {
    COORD cursorPosition;
    cursorPosition.X = x;
    cursorPosition.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

// Konsol imlecini gizleyen fonksiyon
void hideConsoleCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}

// Konsol yazý rengini ayarlayan fonksiyon
void setConsoleTextColor(int colorCode) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colorCode);
}

// Sistemdeki geçerli zamaný mikro saniye cinsinden döndüren fonksiyon
unsigned long long getSystemCurrentTime() {
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    ULARGE_INTEGER systemTime;
    systemTime.LowPart = fileTime.dwLowDateTime;
    systemTime.HighPart = fileTime.dwHighDateTime;
    return systemTime.QuadPart / 10; // Mikro saniye cinsinden
}

// Rastgele bir gecikme süresi üreten fonksiyon
int generateRandomDelayTime(unsigned long int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (*seed % 400) + 600; // 10 ile 50 arasýnda rastgele bir gecikme
}

// Her bir görevin belirtilen süreyi tamamlamasýný simüle eden fonksiyon
void executeTask(int taskIndex, int iterationCount) {
    Sleep(taskDuration[taskIndex]);
    unsigned long int seed = getSystemCurrentTime() + taskIndex + iterationCount + taskDuration[taskIndex];
    // Her on bir iterasyonda görev süresi rastgele olarak deðiþtiriliyor
    if (iterationCount % ((NUM_ITERATIONS / 10) + 1) == 0) {
        taskDuration[taskIndex] = generateRandomDelayTime(&seed);
    }
}

// Görevlerin ilerlemesini ekranda gösteren fonksiyon
void renderProgress() {
    for (int i = 0; i < NUM_TASKS; i++) {
        int progress = taskCompletionStatus[i] * PROGRESS_BAR_HEIGHT / NUM_ITERATIONS;

        // Görev ilerlemesini her satýr için çizme
        for (int j = 0; j < PROGRESS_BAR_HEIGHT; j++) {
            setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT - j);
            if (j < progress) {
                setConsoleTextColor(60); // Ýlerlemiþ kýsmý yeþil renk ile göster
                printf("-");
            } else {
                setConsoleTextColor(46); // Boþ kýsmý gri renk ile göster
                printf(" ");
            }
        }

        setConsoleTextColor(10); // Yazý rengini beyaz yap
        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 1);
        printf("%02d", i + 1); // Görev numarasýný göster

        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 2);
        printf("%3d%%", (taskCompletionStatus[i] * 100) / NUM_ITERATIONS); // Ýlerleme yüzdesi
    }
}

// Görevler tamamlandýðýnda nihai ilerlemeyi gösteren fonksiyon
void renderFinalProgress() {
    for (int i = 0; i < NUM_TASKS; i++) {
        // Her görev için ilerlemesi tamamlanmýþ tüm barlarý doldurma
        for (int j = 0; j < PROGRESS_BAR_HEIGHT; j++) {
            setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT - j);
            setConsoleTextColor(75); // Tamamlanan ilerlemeyi yeþil renk ile göster
            printf("-");
        }

        setConsoleTextColor(7); // Yazý rengini beyaz yap
        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 1);
        printf("%02d", i + 1); // Görev numarasýný göster

        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 2);
        printf("100%%"); // Görev tamamlandýðýný yüzde 100 olarak göster
    }
}

int main() {
    setlocale(LC_ALL, "Turkish"); // Türkçe karakterlerin doðru görünmesini saðla
    int i, j;
    hideConsoleCursor(); // Konsol imlecini gizle

    // Görev baþlangýç ayarlarý
    for (i = 0; i < NUM_TASKS; i++) {
        taskCompletionStatus[i] = 0;
        taskDuration[i] = generateRandomDelayTime((unsigned long int *)&i); // Rastgele görev süreleri oluþtur
    }

    system("cls"); // Konsolu temizle

    // Paralel görev iþleme
    #pragma omp parallel num_threads(NUM_TASKS)
    {
        int taskId = omp_get_thread_num(); // He                                                                      c vvvvvvvvvvvvvvvvvvv  r bir iþ parçacýðýna özgü ID
        taskStartTime[taskId] = getSystemCurrentTime(); // Görev baþlangýç zamanýný al

        for (j = 0; j < NUM_ITERATIONS; j++) {
            if (terminateExecution) break; // Çýkma durumu kontrolü

            executeTask(taskId, j); // Görevi çalýþtýr
            taskCompletionStatus[taskId] = j + 1; // Görev tamamlanma durumunu güncelle

            #pragma omp critical
            renderProgress(); // Ekranda ilerlemeyi güncelle

            if (taskCompletionStatus[taskId] >= NUM_ITERATIONS) {
                taskFinished[taskId] = 1;
                break; // Görev tamamlandýðýnda çýk
            }
        }

        taskEndTime[taskId] = getSystemCurrentTime(); // Görev bitiþ zamanýný al
    }

    renderFinalProgress(); // Tüm görevler tamamlandýðýnda nihai ilerlemeyi göster

    printf("\nTüm görevler tamamlandý.\n");

    // Görev bitiþ zamanlarýný ekrana yazdýr
    printf("\nGörev Tamamlanma Zamanlarý (milisaniye cinsinden):\n");
    for (int i = 0; i < NUM_TASKS; i++) {
        printf("Görev %02d: Baþlangýç Zamaný = %llu ms, Bitiþ Zamaný = %llu ms, Süre = %llu ms\n",
               i + 1, taskStartTime[i] / 1000, taskEndTime[i] / 1000, (taskEndTime[i] - taskStartTime[i]) / 1000);
    }

    setCursorPosition(0, PROGRESS_BAR_HEIGHT + 10); // Son mesaj için konum ayarla
    setConsoleTextColor(7); // Yazý rengini beyaz yap
    printf("Çýkmak için herhangi bir tuþa basýn...");
    getch(); // Kullanýcýnýn tuþa basmasýný bekle

    return 0;
}  
