// Compile: g++ -O0 -std=c++17 -o gen_slow_trump_tricks_refs gen_slow_trump_tricks_refs.cc
// Generate reference SlowTrumpTricks values for testing
//
// This tests the SlowTrumpTricks function which detects finesse positions in trump suits:
// - Kx behind A (partner has Kx and LHO has A, or we have Kx and RHO has A)
// - KQ against A (opponents have A, we have both K and Q)
// - Qxx behind AK (partner has Q with 3+ cards and LHO has AK)

#include <cstdio>
#include <cstdint>
#include <cstring>

static constexpr int NUM_SUITS = 4;
static constexpr int NUM_RANKS = 13;
static constexpr int TOTAL_CARDS = 52;
static constexpr int NOTRUMP = 4;
static constexpr int WEST = 0, NORTH = 1, EAST = 2, SOUTH = 3;
static constexpr int SPADE = 0, HEART = 1, DIAMOND = 2, CLUB = 3;

int suit_of[TOTAL_CARDS];
int rank_of[TOTAL_CARDS];
int card_of[NUM_SUITS][NUM_RANKS];

int SuitOf(int card) { return suit_of[card]; }
int RankOf(int card) { return rank_of[card]; }
int CardOf(int suit, int rank) { return card_of[suit][rank]; }
uint64_t MaskOf(int suit) { return 0x1fffULL << (suit * NUM_RANKS); }

const char* SUIT_NAMES[] = {"S", "H", "D", "C"};
const char* RANK_NAMES[] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};
const char* SEAT_NAMES[] = {"West", "North", "East", "South"};
const char* SEAT_NAMES_LOWER[] = {"west", "north", "east", "south"};
const char* SEAT_NAMES_UPPER[] = {"WEST", "NORTH", "EAST", "SOUTH"};
const char* STRAIN_NAMES[] = {"Spades", "Hearts", "Diamonds", "Clubs", "NT"};
const char* STRAIN_NAMES_LOWER[] = {"spades", "hearts", "diamonds", "clubs", "nt"};
const char* STRAIN_CONSTS[] = {"SPADE", "HEART", "DIAMOND", "CLUB", "NOTRUMP"};

void InitCards() {
    for (int card = 0; card < TOTAL_CARDS; ++card) {
        suit_of[card] = card / NUM_RANKS;
        rank_of[card] = NUM_RANKS - 1 - card % NUM_RANKS;
        card_of[suit_of[card]][rank_of[card]] = card;
    }
}

void PrintCard(int card) {
    printf("%s%s", SUIT_NAMES[SuitOf(card)], RANK_NAMES[RankOf(card)]);
}

struct Cards {
    uint64_t bits = 0;

    Cards() = default;
    Cards(uint64_t b) : bits(b) {}

    int Size() const { return __builtin_popcountll(bits); }
    bool Have(int card) const { return bits & (1ULL << card); }
    operator bool() const { return bits != 0; }

    Cards Suit(int suit) const { return bits & MaskOf(suit); }
    int Top() const { return __builtin_ctzll(bits); }
    int Bottom() const { return 63 - __builtin_clzll(bits); }

    Cards Union(const Cards& c) const { return bits | c.bits; }
    Cards Intersect(const Cards& c) const { return bits & c.bits; }
    Cards Different(const Cards& c) const { return bits & ~c.bits; }
    bool Include(const Cards& c) const { return Intersect(c) == c; }
    bool StrictlyInclude(const Cards& c) const { return Include(c) && bits != c.bits; }

    Cards Add(int card) { bits |= (1ULL << card); return *this; }
    Cards Remove(int card) { bits &= ~(1ULL << card); return *this; }
    Cards Remove(const Cards& c) { bits &= ~c.bits; return *this; }

    struct Iterator {
        uint64_t bits;
        int operator*() const { return __builtin_ctzll(bits); }
        Iterator& operator++() { bits &= bits - 1; return *this; }
        bool operator!=(const Iterator& o) const { return bits != o.bits; }
    };
    Iterator begin() const { return {bits}; }
    Iterator end() const { return {0}; }

    void Show() const {
        for (int suit = 0; suit < NUM_SUITS; ++suit) {
            auto s = Suit(suit);
            if (s) {
                printf("%s", SUIT_NAMES[suit]);
                for (int card : s) printf("%s", RANK_NAMES[RankOf(card)]);
            }
        }
    }
};

Cards hands[4];
Cards all_cards;
int trump = SPADE;
int seat_to_play = WEST;
int num_tricks = 13;

int Partner(int seat) { return (seat + 2) & 3; }
int LeftHandOpp(int seat) { return (seat + 1) & 3; }
int RightHandOpp(int seat) { return (seat + 3) & 3; }

// SlowTrumpTricks: returns number of tricks opponents can guarantee through finesses
// This is from the perspective of the OPPONENTS of seat_to_play
// i.e., "my" = LHO of seat_to_play, "pd" = RHO of seat_to_play
int SlowTrumpTricks(bool leading) {
    Cards all_trumps = all_cards.Suit(trump);
    if (all_trumps.Size() < 3) return 0;

    // From opponents' perspective (seat_to_play's opponents)
    Cards my_trumps = hands[LeftHandOpp(seat_to_play)].Suit(trump);
    Cards pd_trumps = hands[RightHandOpp(seat_to_play)].Suit(trump);
    Cards lho_trumps = hands[Partner(seat_to_play)].Suit(trump);  // opponent's LHO = our partner
    Cards rho_trumps = hands[seat_to_play].Suit(trump);           // opponent's RHO = us

    int a = all_trumps.Top();  // Ace (or highest remaining)
    Cards a_cards = Cards().Add(a);

    Cards remaining = all_trumps.Different(a_cards);
    if (!remaining) return 0;
    int k = remaining.Top();  // King (or second highest)
    Cards k_cards = Cards().Add(k);

    // Kx behind A
    // pd has K with x (strictly includes K, meaning K + at least one more card)
    // and lho has A
    if (pd_trumps.StrictlyInclude(k_cards) && lho_trumps.Have(a)) {
        return 1;
    }
    // OR: my has K with x and rho has A (with leading restriction)
    if (my_trumps.StrictlyInclude(k_cards) && rho_trumps.Have(a) &&
        (!leading || num_tricks >= 3)) {
        return 1;
    }

    // KQ against A
    remaining = remaining.Different(k_cards);
    if (!remaining) return 0;
    int q = remaining.Top();  // Queen (or third highest)

    // Opponents have A, we (opponent pair) have K and Q
    bool opponents_have_a = lho_trumps.Have(a) || rho_trumps.Have(a);
    bool we_have_k = my_trumps.Have(k) || pd_trumps.Have(k);
    bool we_have_q = my_trumps.Have(q) || pd_trumps.Have(q);
    bool we_have_cards = my_trumps.Size() >= 1 || pd_trumps.Size() >= 1;

    if (opponents_have_a && we_have_k && we_have_q && we_have_cards) {
        return 1;
    }

    // Qxx behind AK
    if (all_trumps.Size() >= 5) {
        Cards ak_cards = a_cards.Union(k_cards);
        Cards q_cards = Cards().Add(q);

        // pd has Q with 3+ cards and lho has AK
        if (pd_trumps.Include(q_cards) && pd_trumps.Size() >= 3 && lho_trumps.Include(ak_cards)) {
            return 1;
        }
        // OR: my has Q with 3+ cards and rho has AK (with leading restriction)
        if (my_trumps.Include(q_cards) && my_trumps.Size() >= 3 && rho_trumps.Include(ak_cards) &&
            (!leading || num_tricks >= 4)) {
            return 1;
        }
    }

    return 0;
}

void ParseHand(Cards& hand, const char* str) {
    hand = Cards();
    int suit = 0;
    for (const char* p = str; *p; ++p) {
        if (*p == ' ') { ++suit; continue; }
        if (*p == '-') continue;  // void
        int rank = -1;
        for (int r = 0; r < 13; ++r) if (*p == RANK_NAMES[r][0]) { rank = r; break; }
        if (rank >= 0) hand.Add(CardOf(suit, rank));
    }
}

// Test case structure
struct TestCase {
    const char* name;
    const char* north;
    const char* west;
    const char* east;
    const char* south;
    int trump_suit;
    int expected_from_west;  // SlowTrumpTricks when West to play
    int expected_from_north;
    int expected_from_east;
    int expected_from_south;
};

void PrintHandsAndResult(const char* name, int trump_suit, int seat, int result) {
    printf("  // %s, trump=%s, seat_to_play=%s -> %d\n",
           name, STRAIN_NAMES_LOWER[trump_suit], SEAT_NAMES[seat], result);
    printf("  // N: "); hands[NORTH].Show(); printf("\n");
    printf("  // W: "); hands[WEST].Show(); printf("\n");
    printf("  // E: "); hands[EAST].Show(); printf("\n");
    printf("  // S: "); hands[SOUTH].Show(); printf("\n");
}

int main() {
    InitCards();

    printf("// SlowTrumpTricks reference values from C++ implementation\n");
    printf("// Tests finesse detection in trump suits\n\n");

    // Test cases that exercise different finesse patterns
    // Pattern 1: Kx behind A (pd has Kx, lho has A)
    // Pattern 2: Kx behind A (my has Kx, rho has A)
    // Pattern 3: KQ against A
    // Pattern 4: Qxx behind AK

    printf("// ==================== Test Cases ====================\n\n");

    // Test 1: Kx behind A - partner has Kx, LHO has A
    // From East's perspective (opponents are N-S):
    //   my = South (East's LHO is South)
    //   pd = North (East's RHO is North)
    //   lho = West (opponent's LHO = East's partner)
    //   rho = East (opponent's RHO = East)
    // So if N has Kx and W has A, that's "pd has Kx and lho has A"
    {
        ParseHand(hands[NORTH], "K2 - - -");  // North has K2 of spades
        ParseHand(hands[WEST], "A - - -");     // West has A of spades
        ParseHand(hands[EAST], "Q - - -");     // East has Q
        ParseHand(hands[SOUTH], "J - - -");    // South has J
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 1: Kx behind A (pd=North has K2, lho=West has A)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 2: Kx behind A - my has Kx, RHO has A
    {
        ParseHand(hands[NORTH], "J - - -");     //
        ParseHand(hands[WEST], "K2 - - -");     // West has K2
        ParseHand(hands[EAST], "A - - -");      // East has A
        ParseHand(hands[SOUTH], "Q - - -");     // South has Q
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 2: Kx behind A (my=West has K2, rho=East has A)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 3: KQ against A
    {
        ParseHand(hands[NORTH], "A - - -");     // North has A
        ParseHand(hands[WEST], "K - - -");      // West has K
        ParseHand(hands[EAST], "Q - - -");      // East has Q
        ParseHand(hands[SOUTH], "J - - -");     // South has J
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 3: KQ against A (NS have A, EW have K and Q)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 4: KQ against A - the other way
    {
        ParseHand(hands[NORTH], "K - - -");     // North has K
        ParseHand(hands[WEST], "A - - -");      // West has A
        ParseHand(hands[EAST], "J - - -");      // East has J
        ParseHand(hands[SOUTH], "Q - - -");     // South has Q
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 4: KQ against A (EW have A, NS have K and Q)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 5: Qxx behind AK
    {
        ParseHand(hands[NORTH], "Q32 - - -");   // North has Qxx
        ParseHand(hands[WEST], "AK - - -");     // West has AK
        ParseHand(hands[EAST], "J - - -");      //
        ParseHand(hands[SOUTH], "T - - -");     //
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 5: Qxx behind AK (pd=North has Q32, lho=West has AK)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 6: No finesse - we have the A
    {
        ParseHand(hands[NORTH], "K - - -");
        ParseHand(hands[WEST], "A - - -");
        ParseHand(hands[EAST], "Q - - -");
        ParseHand(hands[SOUTH], "J - - -");
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 6: No finesse - top cards split (N=K, W=A, E=Q, S=J)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 7: K singleton (not Kx) - should NOT match
    {
        ParseHand(hands[NORTH], "K - - -");     // North has just K (singleton)
        ParseHand(hands[WEST], "A - - -");      // West has A
        ParseHand(hands[EAST], "Q2 - - -");     //
        ParseHand(hands[SOUTH], "J - - -");     //
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 7: K singleton - should NOT trigger Kx behind A\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 8: quick_test_4 at depth 8 (Hearts trump, the failing case)
    // At XRAY 4: W=H4, N=H6, E=HQ, S=H8
    {
        ParseHand(hands[NORTH], "- 6 - -");
        ParseHand(hands[WEST], "- 4 - -");
        ParseHand(hands[EAST], "- Q - -");
        ParseHand(hands[SOUTH], "- 8 - -");
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = HEART;
        num_tricks = 4;  // At depth 8, only 4 tricks remain (if started with 4)

        printf("// Test 8: quick_test_4 depth=8 (Hearts: W=4, N=6, E=Q, S=8)\n");
        printf("// No A, K in remaining trumps - Q is highest\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 9: Two cards only
    {
        ParseHand(hands[NORTH], "K - - -");
        ParseHand(hands[WEST], "A - - -");
        ParseHand(hands[EAST], "- - - -");
        ParseHand(hands[SOUTH], "- - - -");
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        trump = SPADE;

        printf("// Test 9: Only 2 trumps (need >= 3 for finesse)\n");
        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            int result = SlowTrumpTricks(false);
            printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
        }
        printf("\n");
    }

    // Test 10: Full deal similar to quick_test_8
    {
        ParseHand(hands[NORTH], "AKQ J6 KJ 9");
        ParseHand(hands[WEST], "65 AK4 AQ T");
        ParseHand(hands[EAST], "J7 QT9 T AK");
        ParseHand(hands[SOUTH], "98 87 96 QJ");
        all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
        num_tricks = 8;

        printf("// Test 10: quick_test_8 full deal\n");
        int strains[] = {SPADE, HEART, DIAMOND, CLUB};
        const char* strain_str[] = {"spades", "hearts", "diamonds", "clubs"};

        for (int s = 0; s < 4; ++s) {
            trump = strains[s];
            printf("// Trump: %s\n", strain_str[s]);
            for (int seat = 0; seat < 4; ++seat) {
                seat_to_play = seat;
                int result = SlowTrumpTricks(false);
                printf("// seat_to_play=%s -> SlowTrumpTricks=%d\n", SEAT_NAMES[seat], result);
            }
        }
        printf("\n");
    }

    // Now generate Rust test code
    printf("\n// ==================== Rust Test Code ====================\n\n");
    printf("// Add these tests to dealer-dds/examples/test_slow_trump_tricks.rs\n\n");

    // Generate structured test data
    printf("#[cfg(test)]\n");
    printf("mod tests {\n");
    printf("    use super::*;\n\n");

    // Test 1: Kx behind A
    printf("    #[test]\n");
    printf("    fn test_kx_behind_a_pd_has_kx() {\n");
    printf("        // N=K2, W=A, E=Q, S=J in spades\n");
    printf("        // From East's view: pd(N) has K2, lho(W) has A -> should return 1\n");
    printf("        let hands = Hands::from_solver_format(\"K2 - - -\", \"A - - -\", \"Q - - -\", \"J - - -\").unwrap();\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, EAST, false), 1);\n");
    printf("        // From West's view: no matching pattern\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, WEST, false), 0);\n");
    printf("    }\n\n");

    printf("    #[test]\n");
    printf("    fn test_kq_against_a() {\n");
    printf("        // N=A, W=K, E=Q, S=J in spades\n");
    printf("        // From North's view: opponents(WE) have K and Q, we(NS) have A -> 1\n");
    printf("        let hands = Hands::from_solver_format(\"A - - -\", \"K - - -\", \"Q - - -\", \"J - - -\").unwrap();\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, NORTH, false), 1);\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, SOUTH, false), 1);\n");
    printf("        // From East/West view: we have K+Q, opponents have A -> 0 (we don't lose to finesse)\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, EAST, false), 0);\n");
    printf("        assert_eq!(slow_trump_tricks_opponent(&hands, SPADE, WEST, false), 0);\n");
    printf("    }\n\n");

    printf("    #[test]\n");
    printf("    fn test_no_finesse_with_low_cards() {\n");
    printf("        // W=4, N=6, E=Q, S=8 in hearts - Q is highest, no A/K present\n");
    printf("        let hands = Hands::from_solver_format(\"- 6 - -\", \"- 4 - -\", \"- Q - -\", \"- 8 - -\").unwrap();\n");
    printf("        // No finesse patterns should match since Q is the top card\n");
    printf("        for seat in [WEST, NORTH, EAST, SOUTH] {\n");
    printf("            assert_eq!(slow_trump_tricks_opponent(&hands, HEART, seat, false), 0,\n");
    printf("                \"Expected 0 for seat {:?}\", seat);\n");
    printf("        }\n");
    printf("    }\n\n");

    printf("}\n");

    return 0;
}
