#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct Node {
    int frame;
    struct Node* next;
};

struct Node* createFreeListRandom(int start, int end) {
    struct Node* head = NULL;
    struct Node* tail = NULL;
    int count = end - start + 1;
    int frames[100];
    int i, j, temp;

    for (i = 0; i < count; i++) {
        frames[i] = start + i;
    }

    for (i = count - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = frames[i];
        frames[i] = frames[j];
        frames[j] = temp;
    }

    for (i = 0; i < count; i++) {
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->frame = frames[i];
        newNode->next = NULL;

        if (head == NULL) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    return head;
}

void displayFreeList(struct Node* head) {
    if (head == NULL) {
        printf("EMPTY\n");
        return;
    }
    struct Node* temp = head;
    while (temp != NULL) {
        printf("%d", temp->frame);
        if (temp->next != NULL)
            printf(" -> ");
        temp = temp->next;
    }
    printf("\n");
}

int allocateFrame(struct Node** head) {
    if (*head == NULL) {
        return -1;
    }
    struct Node* temp = *head;
    int frame = temp->frame;
    *head = temp->next;
    free(temp);
    return frame;
}

void deallocateFrame(struct Node** head, int frame) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->frame = frame;
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
        return;
    }

    struct Node* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}

void displayPageTable(int pageTable[], int numPages, int pageSize) {
    printf("\n--------- PAGE TABLE ---------\n");
    printf("Page No\t\tFrame No\tBase Address\n");
    for (int i = 0; i < numPages; i++) {
        if (pageTable[i] == -1)
            printf("%d\t\tFREE\t\t---\n", i);
        else
            printf("%d\t\t%d\t\t%d\n", i, pageTable[i], pageTable[i] * pageSize);
    }
    printf("------------------------------\n");
}

void logicalToPhysical(int pageTable[], int numPages, int pageSize) {
    int logicalAddr, pageNum, offset, frameNum, physicalAddr;

    printf("\nEnter logical address: ");
    scanf("%d", &logicalAddr);

    if (logicalAddr < 0 || logicalAddr >= numPages * pageSize) {
        printf("Invalid logical address! Must be between 0 and %d\n",
               (numPages * pageSize) - 1);
        return;
    }

    pageNum = logicalAddr / pageSize;
    offset = logicalAddr % pageSize;

    if (pageTable[pageNum] == -1) {
        printf("Page %d is not allocated to any frame!\n", pageNum);
        return;
    }

    frameNum = pageTable[pageNum];
    physicalAddr = (frameNum * pageSize) + offset;

    printf("\n----- ADDRESS TRANSLATION -----\n");
    printf("Logical Address   : %d\n", logicalAddr);
    printf("Page Number       : %d\n", pageNum);
    printf("Offset            : %d\n", offset);
    printf("Frame Number      : %d\n", frameNum);
    printf("Physical Address  : (%d x %d) + %d = %d\n",
           frameNum, pageSize, offset, physicalAddr);
    printf("--------------------------------\n");
}

int main() {
    int processSize, pageSize, numPages, internalFrag;
    int pageTable[100];
    int i, option, frame;
    struct Node* freeList = NULL;

    srand(time(NULL));

    printf("===== PAGING TECHNIQUE WITH FREE FRAME LIST =====\n\n");

    printf("Enter process size (in bytes): ");
    scanf("%d", &processSize);

    printf("Enter page size (in bytes): ");
    scanf("%d", &pageSize);

    numPages = processSize / pageSize;
    if (processSize % pageSize != 0) {
        numPages++;
    }

    internalFrag = (numPages * pageSize) - processSize;

    printf("\nProcess requires %d pages (0 to %d).\n", numPages, numPages - 1);

    freeList = createFreeListRandom(10, 20);

    printf("\nFree Frame List (random order): ");
    displayFreeList(freeList);

    int availableFrames = 0;
    struct Node* temp = freeList;
    while (temp != NULL) {
        availableFrames++;
        temp = temp->next;
    }

    if (numPages > availableFrames) {
        printf("\nError: Process requires %d pages but only %d frames available (10-20).\n",
               numPages, availableFrames);
        return 1;
    }

    printf("\nAllocating frames to pages (delete at front):\n");
    for (i = 0; i < numPages; i++) {
        frame = allocateFrame(&freeList);
        pageTable[i] = frame;
        printf("Page %d -> Frame %d\n", i, frame);
    }

    printf("\n================ MENU ================\n");
    printf("1. Display Page Table\n");
    printf("2. Display Number of Pages & Internal Fragmentation\n");
    printf("3. Logical to Physical Address Translation\n");
    printf("4. Display Free Frame List\n");
    printf("5. Deallocate a Frame (insert at last)\n");
    printf("6. Exit\n");
    printf("======================================\n");

    do {
        printf("\nChoose an option: ");
        scanf("%d", &option);

        switch (option) {
            case 1:
                displayPageTable(pageTable, numPages, pageSize);
                break;

            case 2:
                printf("\n--- PROCESS DETAILS ---\n");
                printf("Process Size           : %d bytes\n", processSize);
                printf("Page Size              : %d bytes\n", pageSize);
                printf("Total Pages Required   : %d\n", numPages);
                printf("Memory Allocated       : %d bytes (%d pages x %d bytes)\n",
                       numPages * pageSize, numPages, pageSize);
                printf("Internal Fragmentation : %d bytes\n", internalFrag);
                if (internalFrag == 0)
                    printf("No internal fragmentation.\n");
                else
                    printf("Wasted space in last page: %d out of %d bytes\n",
                           internalFrag, pageSize);
                break;

            case 3:
                logicalToPhysical(pageTable, numPages, pageSize);
                break;

            case 4:
                printf("\nCurrent Free Frame List: ");
                displayFreeList(freeList);
                break;

            case 5: {
                int pageNum;
                printf("\nEnter page number to deallocate (0 to %d): ", numPages - 1);
                scanf("%d", &pageNum);

                if (pageNum < 0 || pageNum >= numPages) {
                    printf("Invalid page number!\n");
                } else if (pageTable[pageNum] == -1) {
                    printf("Page %d is already deallocated.\n", pageNum);
                } else {
                    frame = pageTable[pageNum];
                    deallocateFrame(&freeList, frame);
                    pageTable[pageNum] = -1;
                    printf("Page %d (Frame %d) deallocated and added to end of free list.\n",
                           pageNum, frame);
                }
                break;
            }

            case 6:
                printf("Exiting program.\n");
                break;

            default:
                printf("\nInvalid option!\n");
        }

    } while (option != 6);

    struct Node* current = freeList;
    while (current != NULL) {
        struct Node* next = current->next;
        free(current);
        current = next;
    }

    return 0;
}