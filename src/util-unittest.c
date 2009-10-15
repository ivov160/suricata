/** Copyright (c) 2009 Open Information Security Foundation
 *
 *  \author Victor Julien <victor@inliniac.net>
 *  \author Breno Silva <breno.silva@gmail.com>
 */


#include "eidps-common.h"
#include "util-unittest.h"

static pcre *parse_regex;
static pcre_extra *parse_regex_study;

static UtTest *ut_list;

/**
 * \brief Allocate UtTest list member
 *
 * \retval ut Pointer to UtTest
 */

static UtTest *UtAllocTest(void) {
    UtTest *ut = malloc(sizeof(UtTest));
    if (ut == NULL) {
        printf("ERROR: UtTest *ut = malloc(sizeof(UtTest)); failed\n");
        return NULL;
    }

    memset(ut, 0, sizeof(UtTest));

    return ut;
}

/**
 * \brief Append test in UtTest list
 *
 * \param list Pointer to the start of the IP packet
 * \param test Pointer to unit test
 *
 * \retval 0 Function always returns zero
 */

static int UtAppendTest(UtTest **list, UtTest *test) {
    if (*list == NULL) {
        *list = test;
    } else {
        UtTest *tmp = *list;

        while (tmp->next != NULL) {
             tmp = tmp->next;
        }
        tmp->next = test;
    }

    return 0;
}

/**
 * \brief Register unit test
 *
 * \param name Unit test name
 * \param TestFn Unit test function
 * \param evalue Unit test function return value
 *
 */

void UtRegisterTest(char *name, int(*TestFn)(void), int evalue) {
    UtTest *ut = UtAllocTest();
    if (ut == NULL)
        return;

    ut->name = name;
    ut->TestFn = TestFn;
    ut->evalue = evalue;
    ut->next = NULL;

    /* append */
    UtAppendTest(&ut_list, ut);
}

/**
 * \brief Compile a regex to run a specific unit test
 *
 * \param regex_arg The regular expression
 *
 * \retval 1  Regex compiled
 * \retval -1 Regex error
 */

int UtRegex (char *regex_arg) {
    const char *eb;
    int eo;
    int opts = 0;

    if(regex_arg == NULL)
        return -1;

    parse_regex = pcre_compile(regex_arg, opts, &eb, &eo, NULL);
    if(parse_regex == NULL)
    {
        printf("pcre compile of \"%s\" failed at offset %" PRId32 ": %s\n", regex_arg, eo, eb);
        goto error;
    }

    parse_regex_study = pcre_study(parse_regex, 0, &eb);
    if(eb != NULL)
    {
        printf("pcre study failed: %s\n", eb);
        goto error;
    }

    return 1;

error:
    return -1;
}

#define MAX_SUBSTRINGS 30

/** \brief List all registered unit tests.
 *
 *  \param regex_arg Regular expression to limit listed tests.
 */
void UtListTests(char *regex_arg) {
    UtTest *ut;
    int ret = 0, rcomp = 0;
    int ov[MAX_SUBSTRINGS];

    rcomp = UtRegex(regex_arg);

    for (ut = ut_list; ut != NULL; ut = ut->next) {
        if (rcomp == 1)  {
            ret = pcre_exec(parse_regex, parse_regex_study, ut->name,
                strlen(ut->name), 0, 0, ov, MAX_SUBSTRINGS);
            if (ret >= 1) {
                printf("%s\n", ut->name);
            }
        }
        else {
            printf("%s\n", ut->name);
        }
    }
}

/** \brief Run all registered unittests.
 *
 *  \param regex_arg The regular expression
 *
 *  \retval 0 all successful
 *  \retval result number of tests that failed
 */

uint32_t UtRunTests(char *regex_arg) {
    UtTest *ut;
    uint32_t good = 0, bad = 0;
    int ret = 0, rcomp = 0;
    int ov[MAX_SUBSTRINGS];

    rcomp = UtRegex(regex_arg);

    for (ut = ut_list; ut != NULL; ut = ut->next) {
        if(rcomp == 1)  {
            ret = pcre_exec(parse_regex, parse_regex_study, ut->name, strlen(ut->name), 0, 0, ov, MAX_SUBSTRINGS);
            if( ret >= 1 )  {
                printf("Test %-60.60s : ", ut->name);
                fflush(stdout); /* flush so in case of a segv we see the testname */
                ret = ut->TestFn();
                printf("%s\n", (ret == ut->evalue) ? "pass" : "FAILED");
                if (ret != ut->evalue) {
                    bad++;
                } else {
                    good++;
                }
            }
        }
        else    {
            printf("Test %-60.60s : ", ut->name);
            fflush(stdout); /* flush so in case of a segv we see the testname */
            ret = ut->TestFn();
            printf("%s\n", (ret == ut->evalue) ? "pass" : "FAILED");
            if (ret != ut->evalue) {
                bad++;
            } else {
                good++;
            }
        }
    }
    printf("==== TEST RESULTS ====\n");
    printf("PASSED: %" PRIu32 "\n", good);
    printf("FAILED: %" PRIu32 "\n", bad);
    printf("======================\n");
    return bad;
}

/**
 * \brief Initialize unit test list
 */

void UtInitialize(void) {
    ut_list = NULL;
}

/**
 * \brief Cleanup unit test list
 */

void UtCleanup(void) {

    UtTest *tmp = ut_list, *otmp;

    while (tmp != NULL) {
        otmp = tmp->next;
        free(tmp);
        tmp = otmp;
    }

    ut_list = NULL;
}

#ifdef UNITTESTS

/** \brief True test
 *
 *  \retval 1 True
 *  \retval 0 False
 */

int UtSelftestTrue(void) {
    if (1)return 1;
    else  return 0;
}

/** \brief False test
 *
 *  \retval 1 False
 *  \retval 0 True
 */

int UtSelftestFalse(void) {
    if (0)return 1;
    else  return 0;
}
#endif /* UNITTESTS */

/** \brief Run self tests
 *
 *  \param regex_arg The regular expression
 *
 *  \retval 0 all successful
 */

int UtRunSelftest (char *regex_arg) {
#ifdef UNITTESTS
    printf("* Running Unittesting subsystem selftests...\n");

    UtInitialize();

    UtRegisterTest("true",  UtSelftestTrue,  1);
    UtRegisterTest("false", UtSelftestFalse, 0);

    int ret = UtRunTests(regex_arg);

    if (ret == 0)
        printf("* Done running Unittesting subsystem selftests...\n");
    else
        printf("* ERROR running Unittesting subsystem selftests failed...\n");

    UtCleanup();
#endif /* UNITTESTS */
    return 0;
}
