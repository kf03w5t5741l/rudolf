#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <curl/curl.h>

typedef struct Response {
    CURLcode code;
    char* data;
    size_t size;
} Response;

/**
  * @brief Callback to store data received by CURL in a single char buffer
  *
  * Based on example provided in CURLOPT_WRITEFUNCTION documentation available
  * at https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html.
  *
  * @param data Pointer to CURL received data
  * @param size Always 1 (see CURL_WRITEFUNCTION documentation)
  * @param nmemb Size of CURL received data in bytes
  * @param userdata Destination for callback output, set by CURLOPT_WRITEDATA
  * @return Bytes processed by callback (equal to nmemb, unless error)
  */
static size_t write_cb(char* data, size_t size, size_t nmemb, void* userdata)
{
    size *= nmemb;
    Response* res = (Response*) userdata;

    char* new_data = realloc(res->data, res->size + size);
    if (!new_data) {
        return 0;
    }

    memcpy(new_data + res->size, data, size);
    res->data = new_data;
    res->size += size;

    return size;
}

/**
  * @brief Get puzzle input from Advent of Code web API
  *
  * @param year
  * @param day
  * @return Pointer to char buffer with puzzle input or NULL on error
  */
static char* api_get_input(int year, int day)
{
    CURL* curl;

    curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    // construct URL to query from base_url and the given year and day
    static const char* base_url = "https://adventofcode.com/%d/day/%d/input";
    size_t url_length = snprintf(NULL, 0, base_url, year, day) + 1;
    char* url = malloc(url_length);
    snprintf(url, url_length, base_url, year, day);

    Response res = { 0 }; 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &res);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_cb);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookie.txt");
    
    res.code = curl_easy_perform(curl);
    if (res.code != CURLE_OK) {
        fprintf(
            stderr, 
            "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res.code)
        );
    }
    free(url);
    curl_easy_cleanup(curl);

    // add null terminator to response data
    char* input = realloc(res.data, res.size + 1);
    if (!input) {
        free(res.data);
        return NULL;
    }
    input[res.size] = 0;

    return input;
}

/**
  * @brief Retrieve puzzle input from local database
  * 
  * @param year
  * @param day
  * @returns Pointer to char buffer containing puzzle input, or NULL on failure
  */
static char* db_get_input(int year, int day)
{
    sqlite3* db;

    int rc = sqlite3_open("input.db", &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    const char query[] = "SELECT input FROM puzzles WHERE year = ? AND day = ?";
    sqlite3_stmt* res;
    rc = sqlite3_prepare_v2(db, query, -1, &res, 0);    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return NULL;
    }
    sqlite3_bind_int(res, 1, year);
    sqlite3_bind_int(res, 2, day);

    rc = sqlite3_step(res);

    char* input = NULL;
    if (rc == SQLITE_ROW) {
        const char* value = (const char*) sqlite3_column_text(res, 0);
        input = calloc(strlen(value) + 1, sizeof(char));
        strncpy(input, value, strlen(value));
    }
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return input;
}

/**
  * @brief Store puzzle input in local database
  * 
  * @param year
  * @param day
  * @param input Pointer to char buffer containing puzzle input
  */ 
void db_put_input(int year, int day, char* input)
{
    sqlite3* db;

    int rc = sqlite3_open("input.db", &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    const char table_query[] = "CREATE TABLE IF NOT EXISTS puzzles (\
        year INTEGER NOT NULL,\
        day INTEGER NOT NULL,\
        input TEXT NOT NULL,\
        PRIMARY KEY (year, day)\
    )";
    sqlite3_stmt* res;
    rc = sqlite3_prepare_v2(db, table_query, -1, &res, 0);    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create table: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return;
    }
    
    rc = sqlite3_step(res);
    sqlite3_finalize(res);

    char insert_query[] = "INSERT INTO puzzles (\
            year,\
            day,\
            input\
        ) VALUES (\
            ?,\
            ?,\
            ?\
        )\
    ";
    rc = sqlite3_prepare_v2(db, insert_query, -1, &res, 0);    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to insert record: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return;
    }
    sqlite3_bind_int(res, 1, year);
    sqlite3_bind_int(res, 2, day);
    sqlite3_bind_text(res, 3, input, -1, SQLITE_STATIC);

    rc = sqlite3_step(res);

    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return;
}

char* rudolf_get_input(int year, int day)
{
    char* input = db_get_input(year, day);
    if (!input) {
        input = api_get_input(year, day);
        db_put_input(year, day, input);
    }

    return input;
}

int main()
{
    char* input = rudolf_get_input(2020, 4);
    printf("%s", input);
    free(input);

    return 0;
}