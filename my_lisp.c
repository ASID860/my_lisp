#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CHECKFUNC(A,B) (!strncmp(d->car->raw,A,d->car->len)&&d->car->len==B)

char mallocsc = 0;
long long mallocs[256] = {0};

enum type {
    none=0, symbol, cons
};

struct cell {
    enum type type;
    union {
        struct {
            struct cell *car, *cdr;
        };
        struct {
            char *raw;
            size_t len;
        };
    };
};

struct cell *eval(struct cell*, struct cell*);
struct cell *apply(struct cell*, struct cell*);

struct cell *getCellAddr() {
    struct cell *out = malloc(sizeof(struct cell));
    memset(out, 0, sizeof(struct cell));

    if (mallocsc < 255) {
        mallocs[mallocsc++] = out;
    }

    return out;
}

void freeAndClean(struct cell *to_free) {
    if (to_free != NULL) {
        memset(to_free, 0, sizeof(struct cell));
        free(to_free);

        int i;
        for (i=0; i < mallocsc ;i++) {
            if (mallocs[i] == to_free) {
                mallocs[i] = 0;
            }
        }
    }
}

void freeCellAddr(struct cell *to_free) {
    if (to_free != NULL) {
        if (to_free -> type == cons) {
            freeCellAddr(to_free -> car);
            freeCellAddr(to_free -> cdr);
        }
        freeAndClean(to_free);
    }
}

struct cell *changeStringToSymbol(char *s) {
    struct cell *out = getCellAddr();
    out -> type = symbol;
    out -> raw = s;
    out -> len = strlen(s);
    return out;
}

struct cell *getCons(struct cell *a, struct cell *b) {
    struct cell *out = getCellAddr();
    out -> type = cons;
    out -> car = a;
    out -> cdr = b;
    return out;
}

struct cell *assoc(struct cell *d, struct cell *list) {
    if (d -> type == cons || list == NULL) {
        return NULL;
    }
    
    if (!strncmp(d -> raw, list -> car -> car -> raw, d -> len) && d -> len == list -> car -> car -> len) {
        return list -> car;
    }
    return assoc(d, list -> cdr);
}

struct cell *map(struct cell *list, struct cell *env) {
    struct cell *out = NULL;
    if (list == NULL) {
        return NULL;
    }
    
    out = getCons(eval(list -> car, env), map(list -> cdr, env));
    return out;
}

struct cell *eval(struct cell *d, struct cell *env) {
    struct cell *out = NULL;
    if (d == NULL) {
        return NULL;
    }
    
    switch (d -> type) {
        case symbol:
            out = assoc(d, env);
            if (out != NULL) {
                out = out -> cdr;
            }
            break;
            
        case cons:
            out = apply(d, env);
            break;
        
        default:
            fprintf(stderr, "[error] not able to eval none\n");
            break;
    }
    return out;
}

struct cell *apply(struct cell *d, struct cell *env) {
    struct cell *out = NULL, *temp, *maptemp;
    switch (d -> type) {
        case cons:
            temp = eval(d -> car, env);
            if (temp != NULL && temp -> type == cons) {
                switch ((temp -> car -> raw)[0] - '0') {
                    case 0:
                        temp = map(d -> cdr, env);
                        
                        if (CHECKFUNC("car", 3)) {
                            out = temp -> car -> car;
                        } else if (CHECKFUNC("cdr", 3)) {
                            out = temp -> car -> cdr;
                        } else if (CHECKFUNC("cons", 4)) {
                            out = getCons(temp -> car, temp -> cdr -> car);
                        } else if (CHECKFUNC("eq", 2)) {
                            if (temp != NULL && temp -> car != NULL && temp -> cdr -> car != NULL && !strncmp(temp -> car -> raw, temp -> cdr -> car -> raw, temp -> car -> len) && temp -> car -> len == temp -> cdr -> car -> len) {
                                out = changeStringToSymbol("t");
                            } else {
                                out = (temp -> car == temp -> cdr -> car ? changeStringToSymbol("t") : NULL);
                            }
                        } else if (CHECKFUNC("atom", 4)) {
                            if (temp -> car == NULL || temp -> car -> type == symbol) {
                                out = changeStringToSymbol("t");
                            } else {
                                out = NULL;
                            }
                        }
                        
                        break;
                    
                    case 1:
                        if (CHECKFUNC("quote", 5)) {
                            out = d -> cdr -> car;
                        } else if (CHECKFUNC("if", 2)) {
                            temp = eval(d -> cdr -> car, env);
                            if (temp != NULL) {
                                out = eval(d -> cdr -> cdr -> car, env);
                            } else {
                                out = eval(d -> cdr -> cdr -> cdr -> car, env);
                            }
                        } else if (CHECKFUNC("lambda", 6)) {
                            out = getCons(changeStringToSymbol("2"), getCons(d -> cdr, getCons(env, NULL)));
                        }
                        break;
                    
                    case 2:
                        maptemp = map(d -> cdr, env);
                        out = eval(temp -> cdr -> car -> cdr -> car, getCons(getCons(temp -> cdr -> car -> car, maptemp -> car), temp -> cdr -> cdr -> car));
                        
                        for (d=d->cdr; d -> cdr != NULL ;d=d->cdr) {
                            maptemp = map(d -> cdr, env);
                            out = eval(out -> cdr -> car -> cdr -> car, getCons(getCons(out -> cdr -> car -> car, maptemp -> car), out -> cdr -> cdr -> car));
                        }
                        break;
                    
                    default: break;
                }
            }
            break;
        
        default:
            fprintf(stderr, "[error] not able to apply none or symbol\n");
            break;
    }
    return out;
}

#ifndef DISABLE_DEBUG
#define ccons getCons
#define csymb changeStringToSymbol

void debugPrint(struct cell *to_print) {
    size_t i;
    if (to_print == NULL) {
        printf("nil");
    } else {
        switch (to_print -> type) {
            case symbol:
                for (i=0; i < to_print -> len ;i++) {
                    putchar(to_print -> raw[i]);
                }
                break;
                
            case cons:
                printf("(");
                debugPrint(to_print -> car);
                printf(" . ");
                debugPrint(to_print -> cdr);
                printf(")");
                break;
                
            default:
                fprintf(stderr, "[error] type is none\n");
                break;
        }
    }
}

struct cell *debugRead(char *s) {
    struct cell *out = NULL, *tmp, *fath = NULL;
    int i;
    while (*s == ' ') s++;
    
    if (*s == '(') {
        for (s=s+1,tmp=NULL;;) {
            while (*s == ' ' || *s == '\n') s++;
            if (*s == '(') {
                out = ccons(debugRead(s), NULL);
                
                if (tmp != NULL) tmp -> cdr = out;
                else fath = out;
                tmp = out;

                for (i=-1; *s != ')' || i ;) {
                    if (*s == '(') i++;
                    else if (*s == ')') i--;
                    s++;
                }
                s++;
            } else if (*s == ')') {
                break;
            } else {
                out = ccons(debugRead(s), NULL);
                
                if (out -> car -> raw[0] == '.' && out -> car -> len == 1) {
                    freeCellAddr(out);
                    out = debugRead(++s);
                    while (*s == ' ' || *s == '\n') s++;
                    fath = ccons(tmp -> car, out);
                    freeAndClean(tmp);
                    break;
                } else {
                    if (tmp != NULL) tmp -> cdr = out;
                    else fath = out;
                    tmp = out;

                    s = s + out -> car -> len;
                }
            }
        }
        out = fath;

    } else {
        for (i=0; ;i++) {
            if (s[i] == '\0' || s[i] == ' ' || s[i] == '\n' || s[i] == ')') {
                if (i == 0) {
                    break;
                }
                out = getCellAddr();
                out -> type = symbol;
                out -> raw = s;
                out -> len = i;
                break;
            }
        }
    }

    return out;
}

#endif

int main() {
    char s[2048];
    struct cell *tmp = NULL, *env;

    env = debugRead("((nil .) (t . t) (car 0) (cdr 0) (cons 0) (eq 0) (atom 0) (quote 1) (if 1) (lambda 1))");

    printf("> ");
    while (fgets(s, sizeof(s)/sizeof(s[0]), stdin)) {
        tmp = debugRead(s);
        debugPrint(eval(tmp, env));
        printf("\n> ");
    }

    freeCellAddr(tmp);
    freeCellAddr(env);
    
    printf("\n");
    for (s[0]=0; s[0] < mallocsc ;s[0]++) {
        mallocs[s[0]] && printf("%p\n", (struct cell*)(mallocs[s[0]]));
    }
    debugRead("");
    return 0;
}
