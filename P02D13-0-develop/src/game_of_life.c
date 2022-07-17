// Copyright 2022 mablever ariesyst isadorat

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define ROW_MAX 25
#define COLUMN_MAX 80
#define SPEED_MULTI 300000.0

typedef struct field_s {
    int **matrix;
    int row, column;
    int speed;
} field_t;

enum place_status {
    void_place = 0,
    cell = 1,
};

enum game_status {
    exit_game = 0,
    continue_game = 1,
};

static struct termios stored_settings;
field_t *init_field(int speed);
void free_field(field_t *field);
void print_field(field_t *field);
void set_keypress(void);
int get_status(int *speed, fd_set *rfds, struct timeval *tv);
int feel_field(field_t *field);
void cp_field(field_t *first_field, field_t *second_field);
int check(field_t *field, int i_pos, int j_pos);
void update(field_t *first_field, field_t *second_field);

int main() {
    int speed = 1;

    fd_set rfds;
    struct timeval tv;

    field_t *field = init_field(speed);
    feel_field(field);
    freopen("/dev/tty", "r", stdin);
    set_keypress();

    while (get_status(&(field->speed), &rfds, &tv)) {
        field_t *field_tmp = init_field(field->speed);
        update(field, field_tmp);
        cp_field(field, field_tmp);
        usleep(SPEED_MULTI / field->speed);
        print_field(field);
        free_field(field_tmp);
    }

    free_field(field);
    return 0;
}

field_t *init_field(int speed) {
    field_t *field = calloc(1, sizeof(field_t));

    field->row = ROW_MAX;
    field->column = COLUMN_MAX;
    field->speed = speed;

    field->matrix = calloc(field->row, sizeof(int *));
    for (int i = 0; i < ROW_MAX; i++) {
        field->matrix[i] = calloc(field->column, sizeof(int));
    }

    return field;
}

int feel_field(field_t *field) {
    for (int i = 0; i < field->row; i++) {
        for (int j = 0; j < field->column; j++) {
            if (scanf("%d", &(field->matrix[i][j])) != 1 ||
                field->matrix[i][j] > 1 || field->matrix[i][j] < 0) {
                return 1;
            }
        }
    }

    return 0;
}

void free_field(field_t *field) {
    for (int i = 0; i < field->row; i++) {
        free(field->matrix[i]);
    }
    free(field->matrix);
    free(field);
}

void print_field(field_t *field) {
    printf("\e[1;1H\e[2J");

    for (int i = 0; i < field->row; i++) {
        for (int j = 0; j < field->column; j++) {
            if (field->matrix[i][j] == void_place) {
                printf(" ");
            } else if (field->matrix[i][j] == cell) {
                printf("@");
            }
        }
        printf("\n");
    }
}

void set_keypress(void) {
    struct termios new_settings;

    tcgetattr(0, &stored_settings);

    new_settings = stored_settings;

    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;

    tcsetattr(0, TCSANOW, &new_settings);
    return;
}

int get_status(int *speed, fd_set *rfds, struct timeval *tv) {
    FD_ZERO(rfds);
    FD_SET(0, rfds);
    tv->tv_sec = 0;
    tv->tv_usec = 0;
    char game_code = ' ';

    int retval = select(2, rfds, NULL, NULL, tv);
    if (retval) {
        game_code = getc(stdin);
    }

    if (game_code == 'w') {
        (*speed)++;
    } else if (game_code == 's') {
        if (*speed != 1) (*speed)--;
    } else if (game_code == 'q' || game_code == 'Q') {
        return exit_game;
    }

    return continue_game;
}

void cp_field(field_t *first_field, field_t *second_field) {
    int **matrix_tmp = first_field->matrix;
    first_field->matrix = second_field->matrix;
    second_field->matrix = matrix_tmp;

    int speed_tmp = first_field->speed;
    first_field->speed = second_field->speed;
    second_field->speed = speed_tmp;
}

void update(field_t *first_field, field_t *second_field) {
    for (int i = 0; i < first_field->row; i++) {
        for (int j = 0; j < first_field->column; j++) {
            int quantity = check(first_field, i, j);
            if (quantity == 3 && first_field->matrix[i][j] == 0) {
                second_field->matrix[i][j] = 1;
            } else if ((quantity < 2 || quantity > 3) &&
                       first_field->matrix[i][j] == 1) {
                second_field->matrix[i][j] = 0;
            } else if ((quantity == 2 || quantity == 3) &&
                       first_field->matrix[i][j] == 1) {
                second_field->matrix[i][j] = 1;
            }
        }
    }
}

int check(field_t *field, int i_pos, int j_pos) {
    int counter = 0;
    for (int i = i_pos - 1; i <= i_pos + 1; i++) {
        for (int j = j_pos - 1; j <= j_pos + 1; j++) {
            int y = i, x = j;
            if (y == -1)
                y = field->row - 1;
            else if (y == field->row)
                y = 0;
            if (x == -1)
                x = field->column - 1;
            else if (x == field->column)
                x = 0;
            if (field->matrix[y][x] == 1) {
                counter++;
            }
        }
    }
    if (field->matrix[i_pos][j_pos] == 1) {
        counter--;
    }
    return counter;
}
