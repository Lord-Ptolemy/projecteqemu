/* -*- buffer-read-only: t -*-
   !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
   This file is built by regcomp.pl from regcomp.sym.
   Any changes made here will be lost!
*/

/* Regops and State definitions */

#define REGNODE_MAX           	90
#define REGMATCH_STATE_MAX    	130

#define	END                   	0	/* 0000 End of program. */
#define	SUCCEED               	1	/* 0x01 Return from a subroutine, basically. */
#define	BOL                   	2	/* 0x02 Match "" at beginning of line. */
#define	MBOL                  	3	/* 0x03 Same, assuming multiline. */
#define	SBOL                  	4	/* 0x04 Same, assuming singleline. */
#define	EOS                   	5	/* 0x05 Match "" at end of string. */
#define	EOL                   	6	/* 0x06 Match "" at end of line. */
#define	MEOL                  	7	/* 0x07 Same, assuming multiline. */
#define	SEOL                  	8	/* 0x08 Same, assuming singleline. */
#define	BOUND                 	9	/* 0x09 Match "" at any word boundary */
#define	BOUNDL                	10	/* 0x0a Match "" at any word boundary */
#define	NBOUND                	11	/* 0x0b Match "" at any word non-boundary */
#define	NBOUNDL               	12	/* 0x0c Match "" at any word non-boundary */
#define	GPOS                  	13	/* 0x0d Matches where last m//g left off. */
#define	REG_ANY               	14	/* 0x0e Match any one character (except newline). */
#define	SANY                  	15	/* 0x0f Match any one character. */
#define	CANY                  	16	/* 0x10 Match any one byte. */
#define	ANYOF                 	17	/* 0x11 Match character in (or not in) this class. */
#define	ALNUM                 	18	/* 0x12 Match any alphanumeric character */
#define	ALNUML                	19	/* 0x13 Match any alphanumeric char in locale */
#define	NALNUM                	20	/* 0x14 Match any non-alphanumeric character */
#define	NALNUML               	21	/* 0x15 Match any non-alphanumeric char in locale */
#define	SPACE                 	22	/* 0x16 Match any whitespace character */
#define	SPACEL                	23	/* 0x17 Match any whitespace char in locale */
#define	NSPACE                	24	/* 0x18 Match any non-whitespace character */
#define	NSPACEL               	25	/* 0x19 Match any non-whitespace char in locale */
#define	DIGIT                 	26	/* 0x1a Match any numeric character */
#define	DIGITL                	27	/* 0x1b Match any numeric character in locale */
#define	NDIGIT                	28	/* 0x1c Match any non-numeric character */
#define	NDIGITL               	29	/* 0x1d Match any non-numeric character in locale */
#define	CLUMP                 	30	/* 0x1e Match any combining character sequence */
#define	BRANCH                	31	/* 0x1f Match this alternative, or the next... */
#define	BACK                  	32	/* 0x20 Match "", "next" ptr points backward. */
#define	EXACT                 	33	/* 0x21 Match this string (preceded by length). */
#define	EXACTF                	34	/* 0x22 Match this string, folded (prec. by length). */
#define	EXACTFL               	35	/* 0x23 Match this string, folded in locale (w/len). */
#define	NOTHING               	36	/* 0x24 Match empty string. */
#define	TAIL                  	37	/* 0x25 Match empty string. Can jump here from outside. */
#define	STAR                  	38	/* 0x26 Match this (simple) thing 0 or more times. */
#define	PLUS                  	39	/* 0x27 Match this (simple) thing 1 or more times. */
#define	CURLY                 	40	/* 0x28 Match this simple thing {n,m} times. */
#define	CURLYN                	41	/* 0x29 Capture next-after-this simple thing */
#define	CURLYM                	42	/* 0x2a Capture this medium-complex thing {n,m} times. */
#define	CURLYX                	43	/* 0x2b Match this complex thing {n,m} times. */
#define	WHILEM                	44	/* 0x2c Do curly processing and see if rest matches. */
#define	OPEN                  	45	/* 0x2d Mark this point in input as start of */
#define	CLOSE                 	46	/* 0x2e Analogous to OPEN. */
#define	REF                   	47	/* 0x2f Match some already matched string */
#define	REFF                  	48	/* 0x30 Match already matched string, folded */
#define	REFFL                 	49	/* 0x31 Match already matched string, folded in loc. */
#define	IFMATCH               	50	/* 0x32 Succeeds if the following matches. */
#define	UNLESSM               	51	/* 0x33 Fails if the following matches. */
#define	SUSPEND               	52	/* 0x34 "Independent" sub-RE. */
#define	IFTHEN                	53	/* 0x35 Switch, should be preceeded by switcher . */
#define	GROUPP                	54	/* 0x36 Whether the group matched. */
#define	LONGJMP               	55	/* 0x37 Jump far away. */
#define	BRANCHJ               	56	/* 0x38 BRANCH with long offset. */
#define	EVAL                  	57	/* 0x39 Execute some Perl code. */
#define	MINMOD                	58	/* 0x3a Next operator is not greedy. */
#define	LOGICAL               	59	/* 0x3b Next opcode should set the flag only. */
#define	RENUM                 	60	/* 0x3c Group with independently numbered parens. */
#define	TRIE                  	61	/* 0x3d Match many EXACT(FL?)? at once. flags==type */
#define	TRIEC                 	62	/* 0x3e Same as TRIE, but with embedded charclass data */
#define	AHOCORASICK           	63	/* 0x3f Aho Corasick stclass. flags==type */
#define	AHOCORASICKC          	64	/* 0x40 Same as AHOCORASICK, but with embedded charclass data */
#define	GOSUB                 	65	/* 0x41 recurse to paren arg1 at (signed) ofs arg2 */
#define	GOSTART               	66	/* 0x42 recurse to start of pattern */
#define	NREF                  	67	/* 0x43 Match some already matched string */
#define	NREFF                 	68	/* 0x44 Match already matched string, folded */
#define	NREFFL                	69	/* 0x45 Match already matched string, folded in loc. */
#define	NGROUPP               	70	/* 0x46 Whether the group matched. */
#define	INSUBP                	71	/* 0x47 Whether we are in a specific recurse. */
#define	DEFINEP               	72	/* 0x48 Never execute directly. */
#define	ENDLIKE               	73	/* 0x49 Used only for the type field of verbs */
#define	OPFAIL                	74	/* 0x4a Same as (?!) */
#define	ACCEPT                	75	/* 0x4b Accepts the current matched string. */
#define	VERB                  	76	/* 0x4c    no-sv 1	Used only for the type field of verbs */
#define	PRUNE                 	77	/* 0x4d Pattern fails at this startpoint if no-backtracking through this */
#define	MARKPOINT             	78	/* 0x4e Push the current location for rollback by cut. */
#define	SKIP                  	79	/* 0x4f On failure skip forward (to the mark) before retrying */
#define	COMMIT                	80	/* 0x50 Pattern fails outright if backtracking through this */
#define	CUTGROUP              	81	/* 0x51 On failure go to the next alternation in the group */
#define	KEEPS                 	82	/* 0x52 $& begins here. */
#define	LNBREAK               	83	/* 0x53 generic newline pattern */
#define	VERTWS                	84	/* 0x54 vertical whitespace         (Perl 6) */
#define	NVERTWS               	85	/* 0x55 not vertical whitespace     (Perl 6) */
#define	HORIZWS               	86	/* 0x56 horizontal whitespace       (Perl 6) */
#define	NHORIZWS              	87	/* 0x57 not horizontal whitespace   (Perl 6) */
#define	FOLDCHAR              	88	/* 0x58 codepoint with tricky case folding properties. */
#define	OPTIMIZED             	89	/* 0x59 Placeholder for dump. */
#define	PSEUDO                	90	/* 0x5a Pseudo opcode for internal use. */
	/* ------------ States ------------- */
#define	TRIE_next             	(REGNODE_MAX + 1)	/* state for TRIE */
#define	TRIE_next_fail        	(REGNODE_MAX + 2)	/* state for TRIE */
#define	EVAL_AB               	(REGNODE_MAX + 3)	/* state for EVAL */
#define	EVAL_AB_fail          	(REGNODE_MAX + 4)	/* state for EVAL */
#define	CURLYX_end            	(REGNODE_MAX + 5)	/* state for CURLYX */
#define	CURLYX_end_fail       	(REGNODE_MAX + 6)	/* state for CURLYX */
#define	WHILEM_A_pre          	(REGNODE_MAX + 7)	/* state for WHILEM */
#define	WHILEM_A_pre_fail     	(REGNODE_MAX + 8)	/* state for WHILEM */
#define	WHILEM_A_min          	(REGNODE_MAX + 9)	/* state for WHILEM */
#define	WHILEM_A_min_fail     	(REGNODE_MAX + 10)	/* state for WHILEM */
#define	WHILEM_A_max          	(REGNODE_MAX + 11)	/* state for WHILEM */
#define	WHILEM_A_max_fail     	(REGNODE_MAX + 12)	/* state for WHILEM */
#define	WHILEM_B_min          	(REGNODE_MAX + 13)	/* state for WHILEM */
#define	WHILEM_B_min_fail     	(REGNODE_MAX + 14)	/* state for WHILEM */
#define	WHILEM_B_max          	(REGNODE_MAX + 15)	/* state for WHILEM */
#define	WHILEM_B_max_fail     	(REGNODE_MAX + 16)	/* state for WHILEM */
#define	BRANCH_next           	(REGNODE_MAX + 17)	/* state for BRANCH */
#define	BRANCH_next_fail      	(REGNODE_MAX + 18)	/* state for BRANCH */
#define	CURLYM_A              	(REGNODE_MAX + 19)	/* state for CURLYM */
#define	CURLYM_A_fail         	(REGNODE_MAX + 20)	/* state for CURLYM */
#define	CURLYM_B              	(REGNODE_MAX + 21)	/* state for CURLYM */
#define	CURLYM_B_fail         	(REGNODE_MAX + 22)	/* state for CURLYM */
#define	IFMATCH_A             	(REGNODE_MAX + 23)	/* state for IFMATCH */
#define	IFMATCH_A_fail        	(REGNODE_MAX + 24)	/* state for IFMATCH */
#define	CURLY_B_min_known     	(REGNODE_MAX + 25)	/* state for CURLY */
#define	CURLY_B_min_known_fail	(REGNODE_MAX + 26)	/* state for CURLY */
#define	CURLY_B_min           	(REGNODE_MAX + 27)	/* state for CURLY */
#define	CURLY_B_min_fail      	(REGNODE_MAX + 28)	/* state for CURLY */
#define	CURLY_B_max           	(REGNODE_MAX + 29)	/* state for CURLY */
#define	CURLY_B_max_fail      	(REGNODE_MAX + 30)	/* state for CURLY */
#define	COMMIT_next           	(REGNODE_MAX + 31)	/* state for COMMIT */
#define	COMMIT_next_fail      	(REGNODE_MAX + 32)	/* state for COMMIT */
#define	MARKPOINT_next        	(REGNODE_MAX + 33)	/* state for MARKPOINT */
#define	MARKPOINT_next_fail   	(REGNODE_MAX + 34)	/* state for MARKPOINT */
#define	SKIP_next             	(REGNODE_MAX + 35)	/* state for SKIP */
#define	SKIP_next_fail        	(REGNODE_MAX + 36)	/* state for SKIP */
#define	CUTGROUP_next         	(REGNODE_MAX + 37)	/* state for CUTGROUP */
#define	CUTGROUP_next_fail    	(REGNODE_MAX + 38)	/* state for CUTGROUP */
#define	KEEPS_next            	(REGNODE_MAX + 39)	/* state for KEEPS */
#define	KEEPS_next_fail       	(REGNODE_MAX + 40)	/* state for KEEPS */

/* PL_regkind[] What type of regop or state is this. */

#ifndef DOINIT
EXTCONST U8 PL_regkind[];
#else
EXTCONST U8 PL_regkind[] = {
	END,      	/* END                    */
	END,      	/* SUCCEED                */
	BOL,      	/* BOL                    */
	BOL,      	/* MBOL                   */
	BOL,      	/* SBOL                   */
	EOL,      	/* EOS                    */
	EOL,      	/* EOL                    */
	EOL,      	/* MEOL                   */
	EOL,      	/* SEOL                   */
	BOUND,    	/* BOUND                  */
	BOUND,    	/* BOUNDL                 */
	NBOUND,   	/* NBOUND                 */
	NBOUND,   	/* NBOUNDL                */
	GPOS,     	/* GPOS                   */
	REG_ANY,  	/* REG_ANY                */
	REG_ANY,  	/* SANY                   */
	REG_ANY,  	/* CANY                   */
	ANYOF,    	/* ANYOF                  */
	ALNUM,    	/* ALNUM                  */
	ALNUM,    	/* ALNUML                 */
	NALNUM,   	/* NALNUM                 */
	NALNUM,   	/* NALNUML                */
	SPACE,    	/* SPACE                  */
	SPACE,    	/* SPACEL                 */
	NSPACE,   	/* NSPACE                 */
	NSPACE,   	/* NSPACEL                */
	DIGIT,    	/* DIGIT                  */
	DIGIT,    	/* DIGITL                 */
	NDIGIT,   	/* NDIGIT                 */
	NDIGIT,   	/* NDIGITL                */
	CLUMP,    	/* CLUMP                  */
	BRANCH,   	/* BRANCH                 */
	BACK,     	/* BACK                   */
	EXACT,    	/* EXACT                  */
	EXACT,    	/* EXACTF                 */
	EXACT,    	/* EXACTFL                */
	NOTHING,  	/* NOTHING                */
	NOTHING,  	/* TAIL                   */
	STAR,     	/* STAR                   */
	PLUS,     	/* PLUS                   */
	CURLY,    	/* CURLY                  */
	CURLY,    	/* CURLYN                 */
	CURLY,    	/* CURLYM                 */
	CURLY,    	/* CURLYX                 */
	WHILEM,   	/* WHILEM                 */
	OPEN,     	/* OPEN                   */
	CLOSE,    	/* CLOSE                  */
	REF,      	/* REF                    */
	REF,      	/* REFF                   */
	REF,      	/* REFFL                  */
	BRANCHJ,  	/* IFMATCH                */
	BRANCHJ,  	/* UNLESSM                */
	BRANCHJ,  	/* SUSPEND                */
	BRANCHJ,  	/* IFTHEN                 */
	GROUPP,   	/* GROUPP                 */
	LONGJMP,  	/* LONGJMP                */
	BRANCHJ,  	/* BRANCHJ                */
	EVAL,     	/* EVAL                   */
	MINMOD,   	/* MINMOD                 */
	LOGICAL,  	/* LOGICAL                */
	BRANCHJ,  	/* RENUM                  */
	TRIE,     	/* TRIE                   */
	TRIE,     	/* TRIEC                  */
	TRIE,     	/* AHOCORASICK            */
	TRIE,     	/* AHOCORASICKC           */
	GOSUB,    	/* GOSUB                  */
	GOSTART,  	/* GOSTART                */
	REF,      	/* NREF                   */
	REF,      	/* NREFF                  */
	REF,      	/* NREFFL                 */
	NGROUPP,  	/* NGROUPP                */
	INSUBP,   	/* INSUBP                 */
	DEFINEP,  	/* DEFINEP                */
	ENDLIKE,  	/* ENDLIKE                */
	ENDLIKE,  	/* OPFAIL                 */
	ENDLIKE,  	/* ACCEPT                 */
	VERB,     	/* VERB                   */
	VERB,     	/* PRUNE                  */
	VERB,     	/* MARKPOINT              */
	VERB,     	/* SKIP                   */
	VERB,     	/* COMMIT                 */
	VERB,     	/* CUTGROUP               */
	KEEPS,    	/* KEEPS                  */
	LNBREAK,  	/* LNBREAK                */
	VERTWS,   	/* VERTWS                 */
	NVERTWS,  	/* NVERTWS                */
	HORIZWS,  	/* HORIZWS                */
	NHORIZWS, 	/* NHORIZWS               */
	FOLDCHAR, 	/* FOLDCHAR               */
	NOTHING,  	/* OPTIMIZED              */
	PSEUDO,   	/* PSEUDO                 */
	/* ------------ States ------------- */
	TRIE,     	/* TRIE_next              */
	TRIE,     	/* TRIE_next_fail         */
	EVAL,     	/* EVAL_AB                */
	EVAL,     	/* EVAL_AB_fail           */
	CURLYX,   	/* CURLYX_end             */
	CURLYX,   	/* CURLYX_end_fail        */
	WHILEM,   	/* WHILEM_A_pre           */
	WHILEM,   	/* WHILEM_A_pre_fail      */
	WHILEM,   	/* WHILEM_A_min           */
	WHILEM,   	/* WHILEM_A_min_fail      */
	WHILEM,   	/* WHILEM_A_max           */
	WHILEM,   	/* WHILEM_A_max_fail      */
	WHILEM,   	/* WHILEM_B_min           */
	WHILEM,   	/* WHILEM_B_min_fail      */
	WHILEM,   	/* WHILEM_B_max           */
	WHILEM,   	/* WHILEM_B_max_fail      */
	BRANCH,   	/* BRANCH_next            */
	BRANCH,   	/* BRANCH_next_fail       */
	CURLYM,   	/* CURLYM_A               */
	CURLYM,   	/* CURLYM_A_fail          */
	CURLYM,   	/* CURLYM_B               */
	CURLYM,   	/* CURLYM_B_fail          */
	IFMATCH,  	/* IFMATCH_A              */
	IFMATCH,  	/* IFMATCH_A_fail         */
	CURLY,    	/* CURLY_B_min_known      */
	CURLY,    	/* CURLY_B_min_known_fail */
	CURLY,    	/* CURLY_B_min            */
	CURLY,    	/* CURLY_B_min_fail       */
	CURLY,    	/* CURLY_B_max            */
	CURLY,    	/* CURLY_B_max_fail       */
	COMMIT,   	/* COMMIT_next            */
	COMMIT,   	/* COMMIT_next_fail       */
	MARKPOINT,	/* MARKPOINT_next         */
	MARKPOINT,	/* MARKPOINT_next_fail    */
	SKIP,     	/* SKIP_next              */
	SKIP,     	/* SKIP_next_fail         */
	CUTGROUP, 	/* CUTGROUP_next          */
	CUTGROUP, 	/* CUTGROUP_next_fail     */
	KEEPS,    	/* KEEPS_next             */
	KEEPS,    	/* KEEPS_next_fail        */
};
#endif

/* regarglen[] - How large is the argument part of the node (in regnodes) */

#ifdef REG_COMP_C
static const U8 regarglen[] = {
	0,                                   	/* END          */
	0,                                   	/* SUCCEED      */
	0,                                   	/* BOL          */
	0,                                   	/* MBOL         */
	0,                                   	/* SBOL         */
	0,                                   	/* EOS          */
	0,                                   	/* EOL          */
	0,                                   	/* MEOL         */
	0,                                   	/* SEOL         */
	0,                                   	/* BOUND        */
	0,                                   	/* BOUNDL       */
	0,                                   	/* NBOUND       */
	0,                                   	/* NBOUNDL      */
	0,                                   	/* GPOS         */
	0,                                   	/* REG_ANY      */
	0,                                   	/* SANY         */
	0,                                   	/* CANY         */
	0,                                   	/* ANYOF        */
	0,                                   	/* ALNUM        */
	0,                                   	/* ALNUML       */
	0,                                   	/* NALNUM       */
	0,                                   	/* NALNUML      */
	0,                                   	/* SPACE        */
	0,                                   	/* SPACEL       */
	0,                                   	/* NSPACE       */
	0,                                   	/* NSPACEL      */
	0,                                   	/* DIGIT        */
	0,                                   	/* DIGITL       */
	0,                                   	/* NDIGIT       */
	0,                                   	/* NDIGITL      */
	0,                                   	/* CLUMP        */
	0,                                   	/* BRANCH       */
	0,                                   	/* BACK         */
	0,                                   	/* EXACT        */
	0,                                   	/* EXACTF       */
	0,                                   	/* EXACTFL      */
	0,                                   	/* NOTHING      */
	0,                                   	/* TAIL         */
	0,                                   	/* STAR         */
	0,                                   	/* PLUS         */
	EXTRA_SIZE(struct regnode_2),        	/* CURLY        */
	EXTRA_SIZE(struct regnode_2),        	/* CURLYN       */
	EXTRA_SIZE(struct regnode_2),        	/* CURLYM       */
	EXTRA_SIZE(struct regnode_2),        	/* CURLYX       */
	0,                                   	/* WHILEM       */
	EXTRA_SIZE(struct regnode_1),        	/* OPEN         */
	EXTRA_SIZE(struct regnode_1),        	/* CLOSE        */
	EXTRA_SIZE(struct regnode_1),        	/* REF          */
	EXTRA_SIZE(struct regnode_1),        	/* REFF         */
	EXTRA_SIZE(struct regnode_1),        	/* REFFL        */
	EXTRA_SIZE(struct regnode_1),        	/* IFMATCH      */
	EXTRA_SIZE(struct regnode_1),        	/* UNLESSM      */
	EXTRA_SIZE(struct regnode_1),        	/* SUSPEND      */
	EXTRA_SIZE(struct regnode_1),        	/* IFTHEN       */
	EXTRA_SIZE(struct regnode_1),        	/* GROUPP       */
	EXTRA_SIZE(struct regnode_1),        	/* LONGJMP      */
	EXTRA_SIZE(struct regnode_1),        	/* BRANCHJ      */
	EXTRA_SIZE(struct regnode_1),        	/* EVAL         */
	0,                                   	/* MINMOD       */
	0,                                   	/* LOGICAL      */
	EXTRA_SIZE(struct regnode_1),        	/* RENUM        */
	EXTRA_SIZE(struct regnode_1),        	/* TRIE         */
	EXTRA_SIZE(struct regnode_charclass),	/* TRIEC        */
	EXTRA_SIZE(struct regnode_1),        	/* AHOCORASICK  */
	EXTRA_SIZE(struct regnode_charclass),	/* AHOCORASICKC */
	EXTRA_SIZE(struct regnode_2L),       	/* GOSUB        */
	0,                                   	/* GOSTART      */
	EXTRA_SIZE(struct regnode_1),        	/* NREF         */
	EXTRA_SIZE(struct regnode_1),        	/* NREFF        */
	EXTRA_SIZE(struct regnode_1),        	/* NREFFL       */
	EXTRA_SIZE(struct regnode_1),        	/* NGROUPP      */
	EXTRA_SIZE(struct regnode_1),        	/* INSUBP       */
	EXTRA_SIZE(struct regnode_1),        	/* DEFINEP      */
	0,                                   	/* ENDLIKE      */
	0,                                   	/* OPFAIL       */
	EXTRA_SIZE(struct regnode_1),        	/* ACCEPT       */
	0,                                   	/* VERB         */
	EXTRA_SIZE(struct regnode_1),        	/* PRUNE        */
	EXTRA_SIZE(struct regnode_1),        	/* MARKPOINT    */
	EXTRA_SIZE(struct regnode_1),        	/* SKIP         */
	EXTRA_SIZE(struct regnode_1),        	/* COMMIT       */
	EXTRA_SIZE(struct regnode_1),        	/* CUTGROUP     */
	0,                                   	/* KEEPS        */
	0,                                   	/* LNBREAK      */
	0,                                   	/* VERTWS       */
	0,                                   	/* NVERTWS      */
	0,                                   	/* HORIZWS      */
	0,                                   	/* NHORIZWS     */
	EXTRA_SIZE(struct regnode_1),        	/* FOLDCHAR     */
	0,                                   	/* OPTIMIZED    */
	0,                                   	/* PSEUDO       */
};

/* reg_off_by_arg[] - Which argument holds the offset to the next node */

static const char reg_off_by_arg[] = {
	0,	/* END          */
	0,	/* SUCCEED      */
	0,	/* BOL          */
	0,	/* MBOL         */
	0,	/* SBOL         */
	0,	/* EOS          */
	0,	/* EOL          */
	0,	/* MEOL         */
	0,	/* SEOL         */
	0,	/* BOUND        */
	0,	/* BOUNDL       */
	0,	/* NBOUND       */
	0,	/* NBOUNDL      */
	0,	/* GPOS         */
	0,	/* REG_ANY      */
	0,	/* SANY         */
	0,	/* CANY         */
	0,	/* ANYOF        */
	0,	/* ALNUM        */
	0,	/* ALNUML       */
	0,	/* NALNUM       */
	0,	/* NALNUML      */
	0,	/* SPACE        */
	0,	/* SPACEL       */
	0,	/* NSPACE       */
	0,	/* NSPACEL      */
	0,	/* DIGIT        */
	0,	/* DIGITL       */
	0,	/* NDIGIT       */
	0,	/* NDIGITL      */
	0,	/* CLUMP        */
	0,	/* BRANCH       */
	0,	/* BACK         */
	0,	/* EXACT        */
	0,	/* EXACTF       */
	0,	/* EXACTFL      */
	0,	/* NOTHING      */
	0,	/* TAIL         */
	0,	/* STAR         */
	0,	/* PLUS         */
	0,	/* CURLY        */
	0,	/* CURLYN       */
	0,	/* CURLYM       */
	0,	/* CURLYX       */
	0,	/* WHILEM       */
	0,	/* OPEN         */
	0,	/* CLOSE        */
	0,	/* REF          */
	0,	/* REFF         */
	0,	/* REFFL        */
	2,	/* IFMATCH      */
	2,	/* UNLESSM      */
	1,	/* SUSPEND      */
	1,	/* IFTHEN       */
	0,	/* GROUPP       */
	1,	/* LONGJMP      */
	1,	/* BRANCHJ      */
	0,	/* EVAL         */
	0,	/* MINMOD       */
	0,	/* LOGICAL      */
	1,	/* RENUM        */
	0,	/* TRIE         */
	0,	/* TRIEC        */
	0,	/* AHOCORASICK  */
	0,	/* AHOCORASICKC */
	0,	/* GOSUB        */
	0,	/* GOSTART      */
	0,	/* NREF         */
	0,	/* NREFF        */
	0,	/* NREFFL       */
	0,	/* NGROUPP      */
	0,	/* INSUBP       */
	0,	/* DEFINEP      */
	0,	/* ENDLIKE      */
	0,	/* OPFAIL       */
	0,	/* ACCEPT       */
	0,	/* VERB         */
	0,	/* PRUNE        */
	0,	/* MARKPOINT    */
	0,	/* SKIP         */
	0,	/* COMMIT       */
	0,	/* CUTGROUP     */
	0,	/* KEEPS        */
	0,	/* LNBREAK      */
	0,	/* VERTWS       */
	0,	/* NVERTWS      */
	0,	/* HORIZWS      */
	0,	/* NHORIZWS     */
	0,	/* FOLDCHAR     */
	0,	/* OPTIMIZED    */
	0,	/* PSEUDO       */
};

#endif /* REG_COMP_C */

/* reg_name[] - Opcode/state names in string form, for debugging */

#ifndef DOINIT
EXTCONST char * PL_reg_name[];
#else
EXTCONST char * const PL_reg_name[] = {
	"END",                   	/* 0000 */
	"SUCCEED",               	/* 0x01 */
	"BOL",                   	/* 0x02 */
	"MBOL",                  	/* 0x03 */
	"SBOL",                  	/* 0x04 */
	"EOS",                   	/* 0x05 */
	"EOL",                   	/* 0x06 */
	"MEOL",                  	/* 0x07 */
	"SEOL",                  	/* 0x08 */
	"BOUND",                 	/* 0x09 */
	"BOUNDL",                	/* 0x0a */
	"NBOUND",                	/* 0x0b */
	"NBOUNDL",               	/* 0x0c */
	"GPOS",                  	/* 0x0d */
	"REG_ANY",               	/* 0x0e */
	"SANY",                  	/* 0x0f */
	"CANY",                  	/* 0x10 */
	"ANYOF",                 	/* 0x11 */
	"ALNUM",                 	/* 0x12 */
	"ALNUML",                	/* 0x13 */
	"NALNUM",                	/* 0x14 */
	"NALNUML",               	/* 0x15 */
	"SPACE",                 	/* 0x16 */
	"SPACEL",                	/* 0x17 */
	"NSPACE",                	/* 0x18 */
	"NSPACEL",               	/* 0x19 */
	"DIGIT",                 	/* 0x1a */
	"DIGITL",                	/* 0x1b */
	"NDIGIT",                	/* 0x1c */
	"NDIGITL",               	/* 0x1d */
	"CLUMP",                 	/* 0x1e */
	"BRANCH",                	/* 0x1f */
	"BACK",                  	/* 0x20 */
	"EXACT",                 	/* 0x21 */
	"EXACTF",                	/* 0x22 */
	"EXACTFL",               	/* 0x23 */
	"NOTHING",               	/* 0x24 */
	"TAIL",                  	/* 0x25 */
	"STAR",                  	/* 0x26 */
	"PLUS",                  	/* 0x27 */
	"CURLY",                 	/* 0x28 */
	"CURLYN",                	/* 0x29 */
	"CURLYM",                	/* 0x2a */
	"CURLYX",                	/* 0x2b */
	"WHILEM",                	/* 0x2c */
	"OPEN",                  	/* 0x2d */
	"CLOSE",                 	/* 0x2e */
	"REF",                   	/* 0x2f */
	"REFF",                  	/* 0x30 */
	"REFFL",                 	/* 0x31 */
	"IFMATCH",               	/* 0x32 */
	"UNLESSM",               	/* 0x33 */
	"SUSPEND",               	/* 0x34 */
	"IFTHEN",                	/* 0x35 */
	"GROUPP",                	/* 0x36 */
	"LONGJMP",               	/* 0x37 */
	"BRANCHJ",               	/* 0x38 */
	"EVAL",                  	/* 0x39 */
	"MINMOD",                	/* 0x3a */
	"LOGICAL",               	/* 0x3b */
	"RENUM",                 	/* 0x3c */
	"TRIE",                  	/* 0x3d */
	"TRIEC",                 	/* 0x3e */
	"AHOCORASICK",           	/* 0x3f */
	"AHOCORASICKC",          	/* 0x40 */
	"GOSUB",                 	/* 0x41 */
	"GOSTART",               	/* 0x42 */
	"NREF",                  	/* 0x43 */
	"NREFF",                 	/* 0x44 */
	"NREFFL",                	/* 0x45 */
	"NGROUPP",               	/* 0x46 */
	"INSUBP",                	/* 0x47 */
	"DEFINEP",               	/* 0x48 */
	"ENDLIKE",               	/* 0x49 */
	"OPFAIL",                	/* 0x4a */
	"ACCEPT",                	/* 0x4b */
	"VERB",                  	/* 0x4c */
	"PRUNE",                 	/* 0x4d */
	"MARKPOINT",             	/* 0x4e */
	"SKIP",                  	/* 0x4f */
	"COMMIT",                	/* 0x50 */
	"CUTGROUP",              	/* 0x51 */
	"KEEPS",                 	/* 0x52 */
	"LNBREAK",               	/* 0x53 */
	"VERTWS",                	/* 0x54 */
	"NVERTWS",               	/* 0x55 */
	"HORIZWS",               	/* 0x56 */
	"NHORIZWS",              	/* 0x57 */
	"FOLDCHAR",              	/* 0x58 */
	"OPTIMIZED",             	/* 0x59 */
	"PSEUDO",                	/* 0x5a */
	/* ------------ States ------------- */
	"TRIE_next",             	/* REGNODE_MAX +0x01 */
	"TRIE_next_fail",        	/* REGNODE_MAX +0x02 */
	"EVAL_AB",               	/* REGNODE_MAX +0x03 */
	"EVAL_AB_fail",          	/* REGNODE_MAX +0x04 */
	"CURLYX_end",            	/* REGNODE_MAX +0x05 */
	"CURLYX_end_fail",       	/* REGNODE_MAX +0x06 */
	"WHILEM_A_pre",          	/* REGNODE_MAX +0x07 */
	"WHILEM_A_pre_fail",     	/* REGNODE_MAX +0x08 */
	"WHILEM_A_min",          	/* REGNODE_MAX +0x09 */
	"WHILEM_A_min_fail",     	/* REGNODE_MAX +0x0a */
	"WHILEM_A_max",          	/* REGNODE_MAX +0x0b */
	"WHILEM_A_max_fail",     	/* REGNODE_MAX +0x0c */
	"WHILEM_B_min",          	/* REGNODE_MAX +0x0d */
	"WHILEM_B_min_fail",     	/* REGNODE_MAX +0x0e */
	"WHILEM_B_max",          	/* REGNODE_MAX +0x0f */
	"WHILEM_B_max_fail",     	/* REGNODE_MAX +0x10 */
	"BRANCH_next",           	/* REGNODE_MAX +0x11 */
	"BRANCH_next_fail",      	/* REGNODE_MAX +0x12 */
	"CURLYM_A",              	/* REGNODE_MAX +0x13 */
	"CURLYM_A_fail",         	/* REGNODE_MAX +0x14 */
	"CURLYM_B",              	/* REGNODE_MAX +0x15 */
	"CURLYM_B_fail",         	/* REGNODE_MAX +0x16 */
	"IFMATCH_A",             	/* REGNODE_MAX +0x17 */
	"IFMATCH_A_fail",        	/* REGNODE_MAX +0x18 */
	"CURLY_B_min_known",     	/* REGNODE_MAX +0x19 */
	"CURLY_B_min_known_fail",	/* REGNODE_MAX +0x1a */
	"CURLY_B_min",           	/* REGNODE_MAX +0x1b */
	"CURLY_B_min_fail",      	/* REGNODE_MAX +0x1c */
	"CURLY_B_max",           	/* REGNODE_MAX +0x1d */
	"CURLY_B_max_fail",      	/* REGNODE_MAX +0x1e */
	"COMMIT_next",           	/* REGNODE_MAX +0x1f */
	"COMMIT_next_fail",      	/* REGNODE_MAX +0x20 */
	"MARKPOINT_next",        	/* REGNODE_MAX +0x21 */
	"MARKPOINT_next_fail",   	/* REGNODE_MAX +0x22 */
	"SKIP_next",             	/* REGNODE_MAX +0x23 */
	"SKIP_next_fail",        	/* REGNODE_MAX +0x24 */
	"CUTGROUP_next",         	/* REGNODE_MAX +0x25 */
	"CUTGROUP_next_fail",    	/* REGNODE_MAX +0x26 */
	"KEEPS_next",            	/* REGNODE_MAX +0x27 */
	"KEEPS_next_fail",       	/* REGNODE_MAX +0x28 */
};
#endif /* DOINIT */

/* PL_reg_extflags_name[] - Opcode/state names in string form, for debugging */

#ifndef DOINIT
EXTCONST char * PL_reg_extflags_name[];
#else
EXTCONST char * const PL_reg_extflags_name[] = {
	/* Bits in extflags defined: 11111111111101111111111100111111 */
	"MULTILINE",        /* 0x00000001 */
	"SINGLELINE",       /* 0x00000002 */
	"FOLD",             /* 0x00000004 */
	"EXTENDED",         /* 0x00000008 */
	"KEEPCOPY",         /* 0x00000010 */
	"LOCALE",           /* 0x00000020 */
	"UNUSED_BIT_6",     /* 0x00000040 */
	"UNUSED_BIT_7",     /* 0x00000080 */
	"ANCH_BOL",         /* 0x00000100 */
	"ANCH_MBOL",        /* 0x00000200 */
	"ANCH_SBOL",        /* 0x00000400 */
	"ANCH_GPOS",        /* 0x00000800 */
	"GPOS_SEEN",        /* 0x00001000 */
	"GPOS_FLOAT",       /* 0x00002000 */
	"LOOKBEHIND_SEEN",  /* 0x00004000 */
	"EVAL_SEEN",        /* 0x00008000 */
	"CANY_SEEN",        /* 0x00010000 */
	"NOSCAN",           /* 0x00020000 */
	"CHECK_ALL",        /* 0x00040000 */
	"UNUSED_BIT_19",    /* 0x00080000 */
	"MATCH_UTF8",       /* 0x00100000 */
	"USE_INTUIT_NOML",  /* 0x00200000 */
	"USE_INTUIT_ML",    /* 0x00400000 */
	"INTUIT_TAIL",      /* 0x00800000 */
	"SPLIT",            /* 0x01000000 */
	"COPY_DONE",        /* 0x02000000 */
	"TAINTED_SEEN",     /* 0x04000000 */
	"TAINTED",          /* 0x08000000 */
	"START_ONLY",       /* 0x10000000 */
	"SKIPWHITE",        /* 0x20000000 */
	"WHITE",            /* 0x40000000 */
	"NULL",             /* 0x80000000 */
};
#endif /* DOINIT */

/* ex: set ro: */
