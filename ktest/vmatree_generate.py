#!/usr/bin/env python
# Generate testcases for vmatree.
# Copyright (C) 2024  dbstream

import random

MIN_ADDR = 0
MAX_ADDR = 9999999

# Sorted array of (first, last) tuples.
arr = []

n = 0

def do_insert ():
	global arr

	i = random.randrange (len (arr) + 1)
	lowest_possible = MIN_ADDR
	highest_possible = MAX_ADDR
	if i > 0:
		lowest_possible = arr[i - 1][1] + 1
	if i < len (arr):
		highest_possible = arr[i][0] - 1

	if lowest_possible > highest_possible:
		return

	if random.choice ((0, 1)) == 1:
		first = random.randrange (lowest_possible, highest_possible + 1)
		last = random.randrange (first, highest_possible + 1)
	else:
		last = random.randrange (lowest_possible, highest_possible + 1)
		first = random.randrange (lowest_possible, last + 1)

	print (f'\tinsert_node ({first}UL, {last}UL);')
	arr = arr[:i] + [(first, last)] + arr[i:]

def do_remove ():
	global arr

	if len (arr) == 0:
		return

	i = random.randrange (len (arr))
	print (f'\tremove_node ({random.randrange (arr[i][0], arr[i][1] + 1)}UL);')
	arr = arr[: i] + arr[i + 1 :]

def do_adjust ():
	global arr

	if len (arr) == 0:
		return

	i = random.randrange (len (arr))
	lowest_possible = MIN_ADDR
	highest_possible = MAX_ADDR
	if i > 0:
		lowest_possible = arr[i - 1][1] + 1
	if i + 1 < len (arr):
		highest_possible = arr[i + 1][0] - 1

	if random.choice ((0, 1)) == 1:
		first = random.randrange (lowest_possible, highest_possible + 1)
		last = random.randrange (first, highest_possible + 1)
	else:
		last = random.randrange (lowest_possible, highest_possible + 1)
		first = random.randrange (lowest_possible, last + 1)

	print (f'\tadjust_node ({random.randrange (arr[i][0], arr[i][1] + 1)}UL, {first}UL, {last}UL);')
	arr[i] = (first, last)

def do_assert_eq ():
	global arr

	i = random.randrange (len (arr) + 1)
	lowest_possible = MIN_ADDR
	highest_possible = MAX_ADDR + 3000
	if i > 0:
		lowest_possible = arr[i - 1][1] + 1
	if i < len (arr):
		highest_possible = arr[i][1]

	addr = random.randrange (lowest_possible, highest_possible + 1)
	if i < len (arr) and addr >= arr[i][0]:
		print (f'\tnum_failed += assert_lookup_eq ({n}, {addr}UL, {arr[i][0]}UL, {arr[i][1]}UL);')
	else:
		print (f'\tnum_failed += assert_lookup_nil ({n}, {addr}UL);')

def align_down (x, align):
	return x - (x % align)

def align_up (x, align):
	return align_down (x + align - 1, align)

def find_free (size, align, low, high, bottomup):
	global arr

	addr = None
	for i in range (len (arr)):
		gap_start = 0
		if i > 0:
			gap_start = arr[i - 1][1] + 1
		gap_end = arr[i][0]

		gap_start = max (low, gap_start)
		if high != None:
			gap_end = min (high, gap_end)

		gap_start = align_up (gap_start, align)
		if gap_start + size <= gap_end:
			if bottomup:
				return gap_start
			addr = align_down (gap_end - size, align)

	if len (arr) > 0:
		gap_start = arr[-1][1] + 1
	else:
		gap_start = 0

	gap_start = max (low, gap_start)
	gap_start = align_up (gap_start, align)
	if high == None:
		if bottomup:
			return gap_start
		return -align_up (size, align)

	gap_end = high + 1
	if gap_start + size <= gap_end:
		if bottomup:
			return gap_start
		addr = align_down (gap_end - size, align)
	return addr

def do_assert_free (bottomup):
	align = 1 << random.randrange (0, 25)
	size = random.randrange (1, 1000000)
	if random.choice ((0, 1)) == 1:
		low = MIN_ADDR
	else:
		low = random.randrange (MIN_ADDR, 10000000)
	if random.choice ((0, 1)) == 1:
		high = None
	else:
		high = random.randrange (MAX_ADDR - 1000000, MAX_ADDR + 1000001)

	addr = find_free (size, align, low, high, bottomup)
	if addr == None:
		print (f'\tnum_failed += assert_{"bottomup" if bottomup else "topdown"}_free_nil ({n}, {size}UL, {align}UL, {low}UL, {high if high != None else "-1"}UL);')
	else:
		print (f'\tnum_failed += assert_{"bottomup" if bottomup else "topdown"}_free_eq ({n}, {size}UL, {align}UL, {low}UL, {high if high != None else "-1"}UL, {addr}UL);')

def do_assert_free_bottomup ():
	do_assert_free (True)

def do_assert_free_topdown ():
	do_assert_free (False)

def test (s):
	if s == 'insert':
		do_insert ()
	elif s == 'remove':
		do_remove ()
	elif s == 'adjust':
		do_adjust ()
	elif s == 'assert_eq':
		do_assert_eq ()
	elif s == 'assert_free_bottomup':
		do_assert_free_bottomup ()
	elif s == 'assert_free_topdown':
		do_assert_free_topdown ()

# have a strong preference for insert operations
choices = [
	'insert',
	'insert',
	'insert',
	'remove',
	'adjust',
	'assert_eq',
	'assert_free_bottomup',
	'assert_free_topdown'
]

for _ in range(10000):
	test (random.choice (choices))
	n += 1

# weaken the preference for insert operations a bit
choices = choices[1:]

for _ in range(10000):
	test (random.choice (choices))
	n += 1

# allow no more modifications
choices = choices[4:]

for _ in range(2000):
	test (random.choice (choices))
	n += 1
