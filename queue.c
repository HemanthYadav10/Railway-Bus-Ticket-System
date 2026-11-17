#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CAPACITY 10
#define MAX_HISTORY 100

typedef struct {
    char name[50];
    char bookingId[25];
    char gender[12];
    char transportType[12];
    char source[30];
    char destination[30];
    char journeyDate[12];
    char departureTime[10];
    int age;
    int tickets;
    int waitingNo;
    int status;
} Passenger;

typedef struct {
    Passenger items[MAX_CAPACITY];
    int front;
    int rear;
    int count;
    Passenger served[MAX_HISTORY];
    Passenger cancelled[MAX_HISTORY];
    int servedCount;
    int cancelledCount;
    int nextWaitingNo;
} Queue;

static Queue gQueue = {0};
static char gDashboard[512] = {0};
static char gRecords[12000] = {0};
static char gSinglePassenger[700] = {0};
static char gQueueString[5000] = {0};

static void copyString(char *dest, size_t size, const char *src) {
    if (dest == NULL || size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, size, "%s", src);
}

static void escapeJson(const char *src, char *dest, size_t size) {
    size_t i = 0;
    size_t j = 0;

    if (src == NULL || dest == NULL || size == 0) {
        return;
    }

    while (src[i] != '\0' && j + 2 < size) {
        char c = src[i];
        switch (c) {
            case '\\':
                dest[j++] = '\\';
                dest[j++] = '\\';
                break;
            case '"':
                dest[j++] = '\\';
                dest[j++] = '"';
                break;
            case '\n':
                dest[j++] = '\\';
                dest[j++] = 'n';
                break;
            case '\r':
                dest[j++] = '\\';
                dest[j++] = 'r';
                break;
            case '\t':
                dest[j++] = '\\';
                dest[j++] = 't';
                break;
            default:
                dest[j++] = c;
                break;
        }
        i++;
    }

    dest[j] = '\0';
}

static const char *statusLabel(int status) {
    if (status == 2) {
        return "Served";
    }
    if (status == 3) {
        return "Cancelled";
    }
    return "Waiting";
}

static void addHistory(Passenger *source, Passenger *history, int *count) {
    if (source == NULL || history == NULL || count == NULL) {
        return;
    }

    if (*count >= MAX_HISTORY) {
        return;
    }

    history[*count] = *source;
    (*count)++;
}

static int findBookingIndex(const Queue *queue, const char *bookingId) {
    if (queue == NULL || bookingId == NULL) {
        return -1;
    }

    for (int i = 0; i < queue->count; i++) {
        int index = (queue->front + i) % MAX_CAPACITY;
        if (strcmp(queue->items[index].bookingId, bookingId) == 0) {
            return index;
        }
    }

    return -1;
}

static int findQueryIndex(const Queue *queue, const char *query) {
    if (queue == NULL || query == NULL) {
        return -1;
    }

    for (int i = 0; i < queue->count; i++) {
        int index = (queue->front + i) % MAX_CAPACITY;
        if (strstr(queue->items[index].bookingId, query) != NULL || strstr(queue->items[index].name, query) != NULL) {
            return index;
        }
    }

    return -1;
}

void queue_reset(void) {
    memset(&gQueue, 0, sizeof(gQueue));
    gQueue.front = 0;
    gQueue.rear = -1;
    gQueue.nextWaitingNo = 1;
}

int queue_is_empty(void) {
    return gQueue.count == 0;
}

int queue_is_full(void) {
    return gQueue.count >= MAX_CAPACITY;
}

int queue_waiting_count(void) {
    return gQueue.count;
}

int queue_served_count(void) {
    return gQueue.servedCount;
}

int queue_cancelled_count(void) {
    return gQueue.cancelledCount;
}

int queue_total_passengers(void) {
    return gQueue.count + gQueue.servedCount + gQueue.cancelledCount;
}

int queue_enqueue(const char *name,
                  const char *bookingId,
                  const char *gender,
                  const char *transportType,
                  const char *source,
                  const char *destination,
                  const char *journeyDate,
                  const char *departureTime,
                  int age,
                  int tickets) {
    if (queue_is_full()) {
        return -1;
    }

    Passenger passenger = {0};
    copyString(passenger.name, sizeof(passenger.name), name);
    copyString(passenger.bookingId, sizeof(passenger.bookingId), bookingId);
    copyString(passenger.gender, sizeof(passenger.gender), gender);
    copyString(passenger.transportType, sizeof(passenger.transportType), transportType);
    copyString(passenger.source, sizeof(passenger.source), source);
    copyString(passenger.destination, sizeof(passenger.destination), destination);
    copyString(passenger.journeyDate, sizeof(passenger.journeyDate), journeyDate);
    copyString(passenger.departureTime, sizeof(passenger.departureTime), departureTime);
    passenger.age = age;
    passenger.tickets = tickets;
    passenger.waitingNo = gQueue.nextWaitingNo++;
    passenger.status = 1;

    if (gQueue.count == 0) {
        gQueue.front = 0;
    }

    gQueue.rear = (gQueue.rear + 1) % MAX_CAPACITY;
    gQueue.items[gQueue.rear] = passenger;
    gQueue.count++;

    return passenger.waitingNo;
}

int queue_dequeue(Passenger *outPassenger) {
    if (queue_is_empty()) {
        return -1;
    }

    Passenger servedPassenger = gQueue.items[gQueue.front];

    for (int i = 0; i < gQueue.count - 1; i++) {
        int currentIndex = (gQueue.front + i) % MAX_CAPACITY;
        int nextIndex = (gQueue.front + i + 1) % MAX_CAPACITY;
        gQueue.items[currentIndex] = gQueue.items[nextIndex];
    }

    gQueue.count--;

    if (gQueue.count == 0) {
        gQueue.front = 0;
        gQueue.rear = -1;
    } else {
        gQueue.rear = (gQueue.rear - 1 + MAX_CAPACITY) % MAX_CAPACITY;
        gQueue.front = 0;
    }

    servedPassenger.status = 2;
    if (outPassenger != NULL) {
        *outPassenger = servedPassenger;
    }

    addHistory(&servedPassenger, gQueue.served, &gQueue.servedCount);
    return 0;
}

int queue_peek(Passenger *outPassenger) {
    if (queue_is_empty()) {
        return -1;
    }

    if (outPassenger != NULL) {
        *outPassenger = gQueue.items[gQueue.front];
    }

    return 0;
}

int queue_cancel_by_booking(const char *bookingId, Passenger *outPassenger) {
    if (bookingId == NULL) {
        return -1;
    }

    int index = findBookingIndex(&gQueue, bookingId);
    if (index < 0) {
        return -1;
    }

    Passenger cancelledPassenger = gQueue.items[index];

    int shiftIndex = 0;
    for (int i = 0; i < gQueue.count; i++) {
        int currentIndex = (gQueue.front + i) % MAX_CAPACITY;
        if (strcmp(gQueue.items[currentIndex].bookingId, bookingId) == 0) {
            shiftIndex = currentIndex;
            break;
        }
    }

    for (int i = shiftIndex; i < MAX_CAPACITY - 1; i++) {
        if (i + 1 < gQueue.count) {
            int currentIndex = (gQueue.front + i) % MAX_CAPACITY;
            int nextIndex = (gQueue.front + i + 1) % MAX_CAPACITY;
            gQueue.items[currentIndex] = gQueue.items[nextIndex];
        }
    }

    gQueue.count--;

    if (gQueue.count == 0) {
        gQueue.front = 0;
        gQueue.rear = -1;
    } else {
        gQueue.rear = (gQueue.rear - 1 + MAX_CAPACITY) % MAX_CAPACITY;
        gQueue.front = 0;
    }

    cancelledPassenger.status = 3;
    if (outPassenger != NULL) {
        *outPassenger = cancelledPassenger;
    }

    addHistory(&cancelledPassenger, gQueue.cancelled, &gQueue.cancelledCount);
    return 0;
}

char *queue_get_dashboard_json(void) {
    snprintf(gDashboard, sizeof(gDashboard),
             "{\"totalPassengers\":%d,\"waitingPassengers\":%d,\"servedPassengers\":%d,\"availableCapacity\":%d,\"queueStatus\":\"%s\"}",
             queue_total_passengers(),
             gQueue.count,
             gQueue.servedCount,
             MAX_CAPACITY - gQueue.count,
             (gQueue.count == 0) ? "Empty Queue" : ((gQueue.count == MAX_CAPACITY) ? "Full Queue" : "Active Queue"));

    return gDashboard;
}

char *queue_get_queue_json(void) {
    memset(gQueueString, 0, sizeof(gQueueString));
    char *cursor = gQueueString;
    size_t remaining = sizeof(gQueueString);

    int written = snprintf(cursor, remaining, "[");
    cursor += written;
    remaining -= (size_t)written;

    for (int i = 0; i < gQueue.count; i++) {
        Passenger current = gQueue.items[(gQueue.front + i) % MAX_CAPACITY];
        char safeName[80] = {0};
        char safeBookingId[50] = {0};
        char safeTransport[20] = {0};
        char safeSource[40] = {0};
        char safeDestination[40] = {0};
        char safeJourney[20] = {0};
        char safeDeparture[15] = {0};

        escapeJson(current.name, safeName, sizeof(safeName));
        escapeJson(current.bookingId, safeBookingId, sizeof(safeBookingId));
        escapeJson(current.transportType, safeTransport, sizeof(safeTransport));
        escapeJson(current.source, safeSource, sizeof(safeSource));
        escapeJson(current.destination, safeDestination, sizeof(safeDestination));
        escapeJson(current.journeyDate, safeJourney, sizeof(safeJourney));
        escapeJson(current.departureTime, safeDeparture, sizeof(safeDeparture));

        written = snprintf(cursor, remaining,
                          "%s{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\",\"age\":%d,\"tickets\":%d}",
                          (i == 0) ? "" : ",",
                          current.waitingNo,
                          safeName,
                          safeBookingId,
                          safeTransport,
                          safeSource,
                          safeDestination,
                          safeJourney,
                          safeDeparture,
                          statusLabel(current.status),
                          current.age,
                          current.tickets);

        cursor += written;
        remaining -= (size_t)written;
    }

    written = snprintf(cursor, remaining, "]");
    cursor += written;
    remaining -= (size_t)written;

    return gQueueString;
}

char *queue_get_all_records_json(void) {
    memset(gRecords, 0, sizeof(gRecords));
    char *cursor = gRecords;
    size_t remaining = sizeof(gRecords);

    int written = snprintf(cursor, remaining, "[");
    cursor += written;
    remaining -= (size_t)written;

    for (int i = 0; i < gQueue.count; i++) {
        Passenger current = gQueue.items[(gQueue.front + i) % MAX_CAPACITY];
        char safeName[80] = {0};
        char safeBookingId[50] = {0};
        char safeTransport[20] = {0};
        char safeSource[40] = {0};
        char safeDestination[40] = {0};
        char safeJourney[20] = {0};
        char safeDeparture[15] = {0};

        escapeJson(current.name, safeName, sizeof(safeName));
        escapeJson(current.bookingId, safeBookingId, sizeof(safeBookingId));
        escapeJson(current.transportType, safeTransport, sizeof(safeTransport));
        escapeJson(current.source, safeSource, sizeof(safeSource));
        escapeJson(current.destination, safeDestination, sizeof(safeDestination));
        escapeJson(current.journeyDate, safeJourney, sizeof(safeJourney));
        escapeJson(current.departureTime, safeDeparture, sizeof(safeDeparture));

        written = snprintf(cursor, remaining,
                          "%s{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\",\"age\":%d,\"tickets\":%d}",
                          (gQueue.servedCount + gQueue.cancelledCount == 0 && i == 0) ? "" : ((i == 0) ? "" : ","),
                          current.waitingNo,
                          safeName,
                          safeBookingId,
                          safeTransport,
                          safeSource,
                          safeDestination,
                          safeJourney,
                          safeDeparture,
                          statusLabel(current.status),
                          current.age,
                          current.tickets);
        cursor += written;
        remaining -= (size_t)written;
    }

    for (int i = 0; i < gQueue.servedCount; i++) {
        Passenger current = gQueue.served[i];
        char safeName[80] = {0};
        char safeBookingId[50] = {0};
        char safeTransport[20] = {0};
        char safeSource[40] = {0};
        char safeDestination[40] = {0};
        char safeJourney[20] = {0};
        char safeDeparture[15] = {0};

        escapeJson(current.name, safeName, sizeof(safeName));
        escapeJson(current.bookingId, safeBookingId, sizeof(safeBookingId));
        escapeJson(current.transportType, safeTransport, sizeof(safeTransport));
        escapeJson(current.source, safeSource, sizeof(safeSource));
        escapeJson(current.destination, safeDestination, sizeof(safeDestination));
        escapeJson(current.journeyDate, safeJourney, sizeof(safeJourney));
        escapeJson(current.departureTime, safeDeparture, sizeof(safeDeparture));

        written = snprintf(cursor, remaining,
                          "%s{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\",\"age\":%d,\"tickets\":%d}",
                          (gQueue.count == 0 && i == 0) ? "" : ",",
                          current.waitingNo,
                          safeName,
                          safeBookingId,
                          safeTransport,
                          safeSource,
                          safeDestination,
                          safeJourney,
                          safeDeparture,
                          statusLabel(current.status),
                          current.age,
                          current.tickets);
        cursor += written;
        remaining -= (size_t)written;
    }

    for (int i = 0; i < gQueue.cancelledCount; i++) {
        Passenger current = gQueue.cancelled[i];
        char safeName[80] = {0};
        char safeBookingId[50] = {0};
        char safeTransport[20] = {0};
        char safeSource[40] = {0};
        char safeDestination[40] = {0};
        char safeJourney[20] = {0};
        char safeDeparture[15] = {0};

        escapeJson(current.name, safeName, sizeof(safeName));
        escapeJson(current.bookingId, safeBookingId, sizeof(safeBookingId));
        escapeJson(current.transportType, safeTransport, sizeof(safeTransport));
        escapeJson(current.source, safeSource, sizeof(safeSource));
        escapeJson(current.destination, safeDestination, sizeof(safeDestination));
        escapeJson(current.journeyDate, safeJourney, sizeof(safeJourney));
        escapeJson(current.departureTime, safeDeparture, sizeof(safeDeparture));

        written = snprintf(cursor, remaining,
                          "%s{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\",\"age\":%d,\"tickets\":%d}",
                          ",",
                          current.waitingNo,
                          safeName,
                          safeBookingId,
                          safeTransport,
                          safeSource,
                          safeDestination,
                          safeJourney,
                          safeDeparture,
                          statusLabel(current.status),
                          current.age,
                          current.tickets);
        cursor += written;
        remaining -= (size_t)written;
    }

    snprintf(cursor, remaining, "]");
    return gRecords;
}

char *queue_peek_json(void) {
    if (queue_is_empty()) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Queue is empty.\"}");
        return gSinglePassenger;
    }

    Passenger frontPassenger = gQueue.items[gQueue.front];
    char safeName[80] = {0};
    char safeBookingId[50] = {0};
    char safeTransport[20] = {0};
    char safeSource[40] = {0};
    char safeDestination[40] = {0};
    char safeJourney[20] = {0};
    char safeDeparture[15] = {0};

    escapeJson(frontPassenger.name, safeName, sizeof(safeName));
    escapeJson(frontPassenger.bookingId, safeBookingId, sizeof(safeBookingId));
    escapeJson(frontPassenger.transportType, safeTransport, sizeof(safeTransport));
    escapeJson(frontPassenger.source, safeSource, sizeof(safeSource));
    escapeJson(frontPassenger.destination, safeDestination, sizeof(safeDestination));
    escapeJson(frontPassenger.journeyDate, safeJourney, sizeof(safeJourney));
    escapeJson(frontPassenger.departureTime, safeDeparture, sizeof(safeDeparture));

    snprintf(gSinglePassenger, sizeof(gSinglePassenger),
             "{\"success\":true,\"passenger\":{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\"}}",
             frontPassenger.waitingNo,
             safeName,
             safeBookingId,
             safeTransport,
             safeSource,
             safeDestination,
             safeJourney,
             safeDeparture,
             statusLabel(frontPassenger.status));

    return gSinglePassenger;
}

char *queue_dequeue_json(void) {
    Passenger servedPassenger;
    if (queue_dequeue(&servedPassenger) == -1) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Queue is empty.\"}");
        return gSinglePassenger;
    }

    char safeName[80] = {0};
    char safeBookingId[50] = {0};
    char safeTransport[20] = {0};
    char safeSource[40] = {0};
    char safeDestination[40] = {0};
    char safeJourney[20] = {0};
    char safeDeparture[15] = {0};

    escapeJson(servedPassenger.name, safeName, sizeof(safeName));
    escapeJson(servedPassenger.bookingId, safeBookingId, sizeof(safeBookingId));
    escapeJson(servedPassenger.transportType, safeTransport, sizeof(safeTransport));
    escapeJson(servedPassenger.source, safeSource, sizeof(safeSource));
    escapeJson(servedPassenger.destination, safeDestination, sizeof(safeDestination));
    escapeJson(servedPassenger.journeyDate, safeJourney, sizeof(safeJourney));
    escapeJson(servedPassenger.departureTime, safeDeparture, sizeof(safeDeparture));

    snprintf(gSinglePassenger, sizeof(gSinglePassenger),
             "{\"success\":true,\"message\":\"Passenger served successfully.\",\"passenger\":{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\"}}",
             servedPassenger.waitingNo,
             safeName,
             safeBookingId,
             safeTransport,
             safeSource,
             safeDestination,
             safeJourney,
             safeDeparture,
             statusLabel(servedPassenger.status));

    return gSinglePassenger;
}

char *queue_search_json(const char *query) {
    if (query == NULL || strlen(query) == 0) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Please enter a name or booking ID.\"}");
        return gSinglePassenger;
    }

    int index = findQueryIndex(&gQueue, query);
    if (index < 0) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Passenger not found.\"}");
        return gSinglePassenger;
    }

    Passenger matchedPassenger = gQueue.items[(gQueue.front + index) % MAX_CAPACITY];
    char safeName[80] = {0};
    char safeBookingId[50] = {0};
    char safeTransport[20] = {0};
    char safeSource[40] = {0};
    char safeDestination[40] = {0};
    char safeJourney[20] = {0};
    char safeDeparture[15] = {0};

    escapeJson(matchedPassenger.name, safeName, sizeof(safeName));
    escapeJson(matchedPassenger.bookingId, safeBookingId, sizeof(safeBookingId));
    escapeJson(matchedPassenger.transportType, safeTransport, sizeof(safeTransport));
    escapeJson(matchedPassenger.source, safeSource, sizeof(safeSource));
    escapeJson(matchedPassenger.destination, safeDestination, sizeof(safeDestination));
    escapeJson(matchedPassenger.journeyDate, safeJourney, sizeof(safeJourney));
    escapeJson(matchedPassenger.departureTime, safeDeparture, sizeof(safeDeparture));

    snprintf(gSinglePassenger, sizeof(gSinglePassenger),
             "{\"success\":true,\"message\":\"Passenger found.\",\"passenger\":{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\"}}",
             matchedPassenger.waitingNo,
             safeName,
             safeBookingId,
             safeTransport,
             safeSource,
             safeDestination,
             safeJourney,
             safeDeparture,
             statusLabel(matchedPassenger.status));

    return gSinglePassenger;
}

char *queue_cancel_by_booking_json(const char *bookingId) {
    if (bookingId == NULL || strlen(bookingId) == 0) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Please provide a booking ID.\"}");
        return gSinglePassenger;
    }

    Passenger cancelledPassenger;
    if (queue_cancel_by_booking(bookingId, &cancelledPassenger) == -1) {
        snprintf(gSinglePassenger, sizeof(gSinglePassenger), "{\"success\":false,\"message\":\"Passenger not found in the waiting list.\"}");
        return gSinglePassenger;
    }

    char safeName[80] = {0};
    char safeBookingId[50] = {0};
    char safeTransport[20] = {0};
    char safeSource[40] = {0};
    char safeDestination[40] = {0};
    char safeJourney[20] = {0};
    char safeDeparture[15] = {0};

    escapeJson(cancelledPassenger.name, safeName, sizeof(safeName));
    escapeJson(cancelledPassenger.bookingId, safeBookingId, sizeof(safeBookingId));
    escapeJson(cancelledPassenger.transportType, safeTransport, sizeof(safeTransport));
    escapeJson(cancelledPassenger.source, safeSource, sizeof(safeSource));
    escapeJson(cancelledPassenger.destination, safeDestination, sizeof(safeDestination));
    escapeJson(cancelledPassenger.journeyDate, safeJourney, sizeof(safeJourney));
    escapeJson(cancelledPassenger.departureTime, safeDeparture, sizeof(safeDeparture));

    snprintf(gSinglePassenger, sizeof(gSinglePassenger),
             "{\"success\":true,\"message\":\"Registration cancelled successfully.\",\"passenger\":{\"waitingNo\":%d,\"name\":\"%s\",\"bookingId\":\"%s\",\"transportType\":\"%s\",\"route\":\"%s - %s\",\"journeyDate\":\"%s\",\"departureTime\":\"%s\",\"status\":\"%s\"}}",
             cancelledPassenger.waitingNo,
             safeName,
             safeBookingId,
             safeTransport,
             safeSource,
             safeDestination,
             safeJourney,
             safeDeparture,
             statusLabel(cancelledPassenger.status));

    return gSinglePassenger;
}

int queue_empty_status(void) {
    return queue_is_empty();
}

int queue_full_status(void) {
    return queue_is_full();
}

char *queue_display_queue(void) {
    return queue_get_queue_json();
}

#ifdef TESTING
int main(void) {
    queue_reset();

    int waiting1 = queue_enqueue("Aisha", "RWL-101", "Female", "Railway", "Dhaka", "Chattogram", "2026-11-23", "08:30", 28, 2);
    int waiting2 = queue_enqueue("Rahim", "BUS-202", "Male", "Bus", "Sylhet", "Dhaka", "2026-11-24", "12:00", 35, 1);
    int waiting3 = queue_enqueue("Nadia", "RWL-103", "Female", "Railway", "Khulna", "Rajshahi", "2026-11-25", "14:15", 24, 1);

    if (waiting1 != 1 || waiting2 != 2 || waiting3 != 3) {
        printf("Unexpected waiting numbers\n");
        return 1;
    }

    Passenger peekPassenger;
    if (queue_peek(&peekPassenger) != 0 || strcmp(peekPassenger.bookingId, "RWL-101") != 0) {
        printf("Peek failed\n");
        return 1;
    }

    Passenger servedPassenger;
    if (queue_dequeue(&servedPassenger) != 0 || strcmp(servedPassenger.bookingId, "RWL-101") != 0) {
        printf("Dequeue failed\n");
        return 1;
    }

    if (queue_cancel_by_booking("BUS-202", NULL) != 0) {
        printf("Cancel failed\n");
        return 1;
    }

    if (queue_is_empty() || queue_waiting_count() != 1 || queue_served_count() != 1 || queue_cancelled_count() != 1) {
        printf("Queue state mismatch\n");
        return 1;
    }

    printf("Queue tests passed\n");
    return 0;
}
#endif
