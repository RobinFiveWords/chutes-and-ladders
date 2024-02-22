import collections
import functools
import random
import sys
import time

import gmpy2
from gmpy2 import mpq
import numpy as np
import sympy
from sympy import Rational

SEED = 'Domingo Montoya'


def timer(func):
    @functools.wraps(func)
    def wrapper(*arg, **kw):
        start = time.time()
        result = func(*arg, **kw)
        duration = time.time() - start
        if duration > 60:
            print('Time for {0}: {1} minutes'
                  .format(func.__name__, round(duration / 60, 1)))
        else:
            print('Time for {0}: {1} seconds'
                  .format(func.__name__, round(duration, 1)))
        return result
    return wrapper


def half_square(rational):
    return rational ** 2 / 2


class Board:
    def __init__(self, text):
        """Chutes and Ladders board description format:
        [start]
        [end]
        [spin_size]
        [from] [to]
        [from] [to]
        ...
        """
        lines = text.strip().split('\n')
        self.start = int(lines[0])
        self.end = int(lines[1])
        self.spin_size = int(lines[2])
        self.jumps = {}
        for line in lines[3:]:
            square_from, square_to = map(int, line.split())
            assert square_from not in self.jumps.keys(), f'multiple jumps from square {square_from}'
            assert square_from not in self.jumps.values(), f'chained jumps at square {square_from}'
            assert square_to not in self.jumps.keys(), f'chained jumps at square {square_to}'
            assert self.start <= square_from <= self.end, f'jump from {square_from} starts outside board'
            assert self.start <= square_to <= self.end, f'jump to {square_to} ends outside board'
            self.jumps[square_from] = square_to

        self.squares = [square for square in range(self.start, self.end + 1)
                        if square not in self.jumps.keys()]
        self.transition_matrix = None
        self.linear_equation_matrix = None
        self.solution = None

    def __repr__(self):
        return f"Board(start={self.start}, end={self.end}; spin_size={self.spin_size}; {len(self.jumps)} jumps)"

    def spin(self):
        return random.randint(1, self.spin_size)

    def move(self, current):
        land = current + self.spin()
        if land > self.end:
            return current
        else:
            return self.jumps.get(land, land)

    def play2(self):
        moves = 0  # note that 1 full turn = 2 moves
        current1 = self.start
        current2 = self.start
        while True:
            current1 = self.move(current1)
            moves += 1
            if current1 == self.end:
                return moves
            current2 = self.move(current2)
            moves += 1
            if current2 == self.end:
                return moves

    def simulate2(self, N, seed=None):
        if seed is not None:
            random.seed(seed)
        results = collections.Counter(self.play2() for _ in range(N))
        return sum(v for (k, v) in results.items() if k % 2 == 1) / N

    def play1(self):
        moves = 0
        current = self.start
        while current != self.end:
            current = self.move(current)
            moves += 1
        return moves

    def results1(self, N, seed=None):
        if seed is not None:
            random.seed(seed)
        return collections.Counter(self.play1() for _ in range(N))

    def simulate1(self, N, seed=None):
        results = self.results1(N, seed=seed)
        return 0.5 + 0.5 * sum(v ** 2 for v in results.values()) / (N ** 2)

    def create_transition_matrix(self):
        m = np.full((len(self.squares), len(self.squares)), Rational(0, 1))
        for i, val in enumerate(self.squares):
            for spin in range(1, self.spin_size + 1):
                land = val + spin
                if land > self.end:
                    j = i
                else:
                    j = self.squares.index(self.jumps.get(land, land))
                m[i, j] += Rational(1, self.spin_size)
        self.transition_matrix = m

    @timer
    def markov(self, moves):
        if self.transition_matrix is None:
            self.create_transition_matrix()
        lower_bound = Rational(1, 2)
        upper_bound = Rational(1, 1)
        state = np.array([Rational(1, 1)] +
                         [Rational(0, 1)] * (self.transition_matrix.shape[0] - 1))
        cumulative = state[-1]
        for move in range(1, moves + 1):
            state = state @ self.transition_matrix
            current_increase = state[-1] - cumulative
            lower_bound += half_square(current_increase)
            remaining = Rational(1, 1) - state[-1]
            upper_bound = lower_bound + half_square(remaining)
            cumulative = state[-1]
        return lower_bound, upper_bound


def get_index(i, j, length):
    """Assign a unique index for each entry in `length`-by-`length` array."""
    return i * length + j


def create_linear_equations_matrix(board):
    squares = board.squares[:-1]  # excludes board.end
    length = len(squares)
    entries = collections.defaultdict(int)
    for i, v1 in enumerate(squares):
        for j, v2 in enumerate(squares):
            row_index = get_index(i, j, length)
            entries[(row_index, row_index)] = mpq(board.spin_size, 1)
            for spin in range(1, board.spin_size + 1):
                land = v1 + spin
                if land > board.end:
                    new_v1 = v1
                    new_i = i
                else:
                    new_v1 = board.jumps.get(land, land)
                    if new_v1 == board.end:
                        continue
                    new_i = board.squares.index(new_v1)
                col_index = get_index(j, new_i, length)
                entries[(row_index, col_index)] += mpq(1, 1)
    return entries


@timer
def solve(board, linear_equations_matrix=None):
    length = len(board.squares) - 1
    row_count = length * length
    rows = []
    above = []
    for _ in range(row_count):
        rows.append(collections.defaultdict(int))
        above.append(collections.defaultdict(int))

    if linear_equations_matrix is None:
        linear_equations_matrix = create_linear_equations_matrix(board)
    for (r, c), value in linear_equations_matrix.items():
        rows[r][c] = value
        if r < c:
            above[c][r] = value

    for r in range(row_count):
        rows[r][row_count] = board.spin_size

    for rev_c, col_d_above in enumerate(reversed(above)):  # right to left
        c = row_count - 1 - rev_c
        row_from = rows[c]
        diagonal_value = row_from[c]
        while col_d_above:
            r, value = col_d_above.popitem()
            factor = value / diagonal_value
            row_to = rows[r]
            for col, from_val in row_from.items():
                if col == c:
                    del row_to[col]
                    continue
                adjustment = factor * from_val
                row_to[col] -= adjustment
                if r < col < row_count:
                    above[col][r] = row_to[col]
    assert not any(above)

    return rows[0][row_count] / rows[0][0]


board = Board("""
0
100
6
1 38
4 14
9 31
21 42
28 84
36 44
51 67
71 91
80 100
98 78
95 75
93 73
87 24
64 60
62 19
56 53
49 11
48 26
16 6
""")

board_10 = Board("""
0
10
6
4 7
9 2
""")


def main():
    return board.exact()


if __name__ == '__main__':
    print(main())
