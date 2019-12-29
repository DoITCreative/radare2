#include <r_anal.h>
#include "minunit.h"

static bool check_invariants(RAnal *anal) {
	RBIter iter;
	RAnalBlock *block;
	ut64 last_start = UT64_MAX;
	ut64 last_end = 0;
	r_rbtree_foreach (anal->bb_tree, iter, block, RAnalBlock, _rb) {
		mu_assert ("overlapping block", block->addr >= last_end);
		if (last_start != UT64_MAX) {
			mu_assert ("corrupted binary tree", block->addr >= last_start);
			mu_assert_neq (block->addr, last_start, "double blocks");
		}
		last_start = block->addr;
		last_end = block->addr + block->size;

		mu_assert ("block->ref < 1, but it is still in the tree", block->ref >= 1);
		mu_assert ("block->ref < r_list_length (block->fcns)", block->ref >= r_list_length (block->fcns));

		RListIter *fcniter;
		RAnalFunction *fcn;
		r_list_foreach (block->fcns, fcniter, fcn) {
			RListIter *fcniter2;
			RAnalFunction *fcn2;
			for (fcniter2 = fcniter->n; fcniter2 && (fcn2 = fcniter2->data, 1); fcniter2 = fcniter2->n) {
				mu_assert_ptrneq (fcn, fcn2, "duplicate function in basic block");
			}
			mu_assert ("block references function, but function does not reference block", r_list_contains (fcn->bbs, block));
		}
	}

	RListIter *fcniter;
	RAnalFunction *fcn;
	r_list_foreach (anal->fcns, fcniter, fcn) {
		RListIter *blockiter;
		ut64 min = UT64_MAX;
		ut64 max = UT64_MIN;
		ut64 realsz = 0;
		r_list_foreach (fcn->bbs, blockiter, block) {
			RListIter *blockiter2;
			RAnalBlock *block2;
			if (block->addr < min) {
				min = block->addr;
			}
			if (block->addr + block->size > max) {
				max = block->addr + block->size;
			}
			realsz += block->size;
			for (blockiter2 = blockiter->n; blockiter2 && (block2 = blockiter2->data, 1); blockiter2 = blockiter2->n) {
				mu_assert_ptrneq (block, block2, "duplicate basic block in function");
			}
			mu_assert ("function references block, but block does not reference function", r_list_contains (block->fcns, fcn));
		}

		if (fcn->meta._min != UT64_MAX) {
			mu_assert_eq (fcn->meta._min, min, "function min wrong");
			mu_assert_eq (fcn->meta._max, max, "function max wrong");
		}

		mu_assert_eq (r_anal_fcn_realsize (fcn), realsz, "realsize wrong");
	}
	return true;
}

R_API bool check_leaks(RAnal *anal) {
	RBIter iter;
	RAnalBlock *block;
	r_rbtree_foreach (anal->bb_tree, iter, block, RAnalBlock, _rb) {
		if (block->ref != r_list_length (block->fcns))  {
			mu_assert ("leaked basic block", false);
		}
	}
	return true;
}

#define assert_invariants(anal) do { if (!check_invariants (anal)) { return false; } } while (0)
#define assert_leaks(anal) do { if (!check_leaks (anal)) { return false; } } while (0)

bool test_r_anal_block_create(void) {
	RAnal *anal = r_anal_new ();
	assert_invariants (anal);

	RList *created = r_anal_block_create (anal, 0x1337, 42);
	assert_invariants (anal);
	mu_assert_eq (r_list_length (created), 1, "created single");
	RAnalBlock *block = r_list_first (created);
	mu_assert_eq (block->addr, 0x1337, "created addr");
	mu_assert_eq (block->size, 42, "created size");
	r_anal_block_unref (block);

	assert_leaks (anal);
	r_anal_free (anal);
	mu_end;
}

bool test_r_anal_block_resize_atomic(void) {
	RAnal *anal = r_anal_new ();
	assert_invariants (anal);

	// TODO

	assert_invariants (anal);
	r_anal_free (anal);
	mu_end;
}

int all_tests() {
	mu_run_test(test_r_anal_block_create);
	mu_run_test(test_r_anal_block_resize_atomic);
	return tests_passed != tests_run;
}

int main(int argc, char **argv) {
	return all_tests();
}
