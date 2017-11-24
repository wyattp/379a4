#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <time.h>
#include <fcntl.h>

#define PRINT(str) printf ("%s\n", str)

/* 
 * reference string operations
 */
#define OP_INC  0
#define OP_DEC  1
#define OP_WR   2
#define OP_RD   3

#define TEST_NPAGES 20
#define PROG_NAME "av4sim"

/*
 * reference string bitfield
 */
struct ref {
    unsigned int    val:    6;
    unsigned int    op :    2;
    unsigned int    unused : 2;
    unsigned int    pg :    22;
};

/*
 * page frame
 */
struct frame {
    int             used;
    int             dirty;
    int             ref;
    unsigned int    pg;
    int             last_use;
};

/*
 * stats structure
 */
struct vm_stats {
    int     writes;
    int     faults;
    int     accum;
    int     flushes;
    clock_t start;
    clock_t end;
};

/*
 * functions
 */
void    read_page (int page);
void    write_page (int page);
double  pr_time (clock_t real);
void    insert_page (int pos, int page, int last_use);

/* replacement straegies */
struct frame * search_mem (int page); 
struct frame * none_replacement (int page);
struct frame * mrand_replacement (int page);
struct frame * lru_replacement (int page);
struct frame * sec_replacement (int page);

/*
 * global vars
 */
int pagesize;
int memsize;
int npages;
int rstrat;
int ref_num;

struct frame *  page_frames;
struct ref      reference;

struct vm_stats stats = {0};

struct frame * (*replace)(int);

void
usage (void) {
    printf ("Usage:\t./a4vsim pagesize memsize strategy\n"
            "\n"
            "\tPagesize is between 256 and 8192\n"
            "\tStrategy is one of 'none', 'mrand', 'lru', 'sec'\n");
}

int
main (int argc, char *argv[]) {
    int err;
    char *strat;

    /*
     * params
     */
    if (argc < 4) {
        usage();
        return -1;
    }

    pagesize = atoi (argv[1]);
    memsize = atoi (argv[2]);
    strat = argv[3];

    if (pagesize > 8192 || pagesize < 256) {
        printf ("Value of pagesize needs to be between 256 and 8192 bytes\n");
        return -1;
    }

    npages = (memsize/pagesize + (memsize % pagesize != 0));
    memsize = npages * pagesize;

    /* set replacement strategy */
    if ((err = !strcmp ("none", strat))) {
        npages = TEST_NPAGES;
        replace = &none_replacement;
    }
    if (!err && (err = !strcmp ("mrand", strat)))
        replace = &mrand_replacement;
    if (!err && (err = !strcmp ("lru", strat)))
        replace = &lru_replacement;
    if (!err && (err = !strcmp ("sec", strat)))
        replace = &sec_replacement;

    if (!err) {
        printf ("Page replacement strategy needs to be one of: none, mrand, lru, sec\n");
        return -1;
    }

    /*
     * start sim
     */
    printf ("a4vsim [page= %d, mem= %d, %s, page num= %d]\n"
            , pagesize, memsize, strat, npages);

    page_frames = malloc ((npages) * sizeof (struct frame));
    ref_num = 0;

    stats.start = clock();
    while (read (STDIN_FILENO, &reference, 8) > 0) {

        switch (reference.op) {

        case OP_INC:
            read_page (reference.pg);
            stats.accum += reference.val;
            break;

        case OP_DEC:
            read_page (reference.pg);
            stats.accum -= reference.val;
            break;
            
        case OP_WR:
            //printf ("%d\n",reference.pg);
            write_page (reference.pg);
            stats.writes += 1;
            break;

        case OP_RD:
            read_page (reference.pg);
            break;

        default:
            fprintf (stderr, "[%s] Error: Illegal opcode, received %d\n"
                    ,PROG_NAME, reference.op);

        }
        ref_num++;

    }
    stats.end = clock();

    printf ("[%s] %d references processed using '%s' in %.2f sec.\n"
            , PROG_NAME, ref_num, strat, pr_time(stats.end-stats.start));
    printf ("[%s] page faults= %d, write count= %d, flushes= %d\n"
            , PROG_NAME, stats.faults, stats.writes, stats.flushes);
    printf ("[%s] Accumulator= %d\n", PROG_NAME, stats.accum);

    /* int i;
    for (i = 0; i < npages; i++) {
        printf ("%d\n", page_frames[i].pg);
    } */
    
    free (page_frames);

    return 0;
}

/*
 * attempt to read page, if page is not in memory
 * throw a page fault and bring into memory and execute
 * the replacement algorithm
 */
void
read_page (int page) {
    struct frame *fr;

    fr = search_mem(page);

    if (fr == NULL) {
        stats.faults++;
        fr = replace (page);
    }

    fr->last_use = ref_num;
    fr->ref = 1;
}

/*
 * attempt to write page, if page is not in memory
 * throw a page fault and bring into memory and execute
 * the replacement algorithm then write to the page
 */
void
write_page (int page) {
    struct frame *fr;
    
    fr = search_mem(page);

    if (fr == NULL) {
        stats.faults++;
        fr = replace (page);
    }

    fr->last_use = ref_num;
    fr->dirty = 1;
    fr->ref = 1;
}

/*
 * make a reference to a page. bring this frame into
 * memory if the page is not already memory resident
 *
 * returns NULL if no page is found, otherwise return
 * a pointer to the current frame in memory
 */
struct frame *
search_mem (int page) {
    int i;

    for (i = 0; i < npages; i++) {
        
        if (page_frames[i].pg == page)
            return &page_frames[i];

    }

    return NULL;
}

/*
 * NOTE: replace () is a pointer to one of the following
 */

struct frame *
none_replacement (int page) {
    static  int i = 0;

    if (i >= npages) {
        npages += 1000;
        page_frames = realloc (page_frames, (npages)*sizeof (struct frame));
    }

    insert_page (i, page, ref_num);

    return &page_frames[i++];
}

struct frame *
mrand_replacement (int page) {

}

/*
 * least recently used page replacement
 */
struct frame *
lru_replacement (int page) {
    int i, ref_min = -1, ref_min_index;

    for (i = 0; i < npages && page_frames[i].used > 0; i++) {
        if (ref_min < 0 || page_frames[i].last_use < ref_min) {
            ref_min = page_frames[i].last_use;
            ref_min_index = i;
        }
    }

    if (i == npages)        /* no empty frame found */
        i = ref_min_index;

    if (page_frames[i].dirty)
        stats.flushes++;

    /* add page */
    insert_page (i, page, ref_num);

    return &page_frames[i];
}

/*
 * second chance replacement
 */
struct frame *
sec_replacement (int page) {
    static int  pos = 0;
    static int  full = 0;

    /* page table full, go through reference bits */
    while (full && page_frames[pos].ref == 1) {
        page_frames[pos].ref = 0;
        if (++pos >= npages)
            pos = 0;
    }

    if (page_frames[pos].dirty)
        stats.flushes++;

    /* add page */
    insert_page (pos, page, ref_num);

    /* page table not full */
    if (!full) {
        if (++pos >= npages) {
            pos = 0;
            full = 1;
        }
    }

    return &page_frames[pos];
}

void
insert_page (int pos, int page, int last_use) {

    page_frames[pos].pg = page;
    page_frames[pos].used = 1;
    page_frames[pos].dirty = 0;
    page_frames[pos].last_use = last_use;
    page_frames[pos].ref = 0;

}

double
pr_time (clock_t real) {
    return real / (double) CLOCKS_PER_SEC;
}
