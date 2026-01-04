// Compile: g++ -O0 -std=c++17 -o gen_follow_order_refs gen_follow_order_refs.cc
// Generate reference follow ordering for testing

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

    // Slice returns cards from start (inclusive) to end (exclusive)
    // For cards where lower index = higher rank
    Cards Slice(int start, int end) const {
        if (start >= end) return Cards();
        uint64_t mask = ((1ULL << end) - 1) & ~((1ULL << start) - 1);
        return bits & mask;
    }

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

    // Reverse iterator for adding cards in reverse order (low to high)
    struct ReverseIterator {
        uint64_t bits;
        int operator*() const { return 63 - __builtin_clzll(bits); }
        ReverseIterator& operator++() { bits &= ~(1ULL << (63 - __builtin_clzll(bits))); return *this; }
        bool operator!=(const ReverseIterator& o) const { return bits != o.bits; }
    };
    ReverseIterator rbegin() const { return {bits}; }
    ReverseIterator rend() const { return {0}; }
};

Cards hands[4];
Cards all_cards;
int trump = NOTRUMP;
int seat_to_play = WEST;
int lead_suit = SPADE;
int winning_card = 0;
int winning_seat = WEST;
int card_in_trick = 1;  // 0=lead, 1=second, 2=third, 3=fourth

int Partner(int seat) { return (seat + 2) & 3; }
int LeftHandOpp(int seat) { return (seat + 1) & 3; }
int RightHandOpp(int seat) { return (seat + 3) & 3; }

bool HigherRank(int c1, int c2) { return c1 < c2; }

bool WinOver(int c1, int c2) {
    return SuitOf(c1) == SuitOf(c2) ? HigherRank(c1, c2) : SuitOf(c1) == trump;
}

char ordered_cards[52];
int num_ordered = 0;

void Reset() { num_ordered = 0; }
void AddCard(int card) { ordered_cards[num_ordered++] = card; }
void AddCards(Cards cards) { for (int c : cards) AddCard(c); }
void AddReversedCards(Cards cards) {
    for (auto it = cards.rbegin(); it != cards.rend(); ++it) AddCard(*it);
}

// Get playable cards for following - must follow suit if possible
Cards GetPlayableCards(int seat, int suit) {
    Cards hand = hands[seat];
    Cards suit_cards = hand.Suit(suit);
    if (suit_cards) {
        return suit_cards;  // Must follow suit
    }
    return hand;  // Void in suit - can play anything
}

// Follow logic from solver.cc
void Follow(Cards playable_cards) {
    bool trick_ending = (card_in_trick == 3);
    bool second_seat = (card_in_trick == 1);

    Cards pd_suit = hands[Partner(seat_to_play)].Suit(lead_suit);
    Cards lho_suit = hands[LeftHandOpp(seat_to_play)].Suit(lead_suit);

    // Following suit?
    if (playable_cards.Suit(lead_suit)) {
        Cards my_suit = playable_cards.Suit(lead_suit);

        // Can't beat current winner - play low first
        if (!WinOver(my_suit.Top(), winning_card)) {
            AddReversedCards(playable_cards);
            return;
        }

        // Partner is winning
        if (winning_seat == Partner(seat_to_play) &&
            (trick_ending || !lho_suit || HigherRank(winning_card, lho_suit.Top()) ||
             lho_suit.Slice(0, winning_card) == lho_suit.Slice(0, my_suit.Top()))) {
            // Partner can win or force LHO's winner
            AddReversedCards(playable_cards);
            return;
        }

        // Second seat analysis
        if (second_seat && pd_suit && HigherRank(pd_suit.Top(), winning_card)) {
            if (lho_suit && HigherRank(lho_suit.Top(), pd_suit.Union(my_suit).Top()) &&
                lho_suit.Slice(0, pd_suit.Top()) == lho_suit.Slice(0, my_suit.Top())) {
                // Play low as LHO may play winner to prevent partner from winning
                AddReversedCards(playable_cards);
                return;
            }
            if (!lho_suit || HigherRank(pd_suit.Top(), lho_suit.Top())) {
                // Play low as partner can win later
                AddReversedCards(playable_cards);
                return;
            }
        }

        // Split into higher and lower cards relative to winning card
        Cards higher_cards = my_suit.Slice(0, winning_card);
        if (trick_ending || !lho_suit || HigherRank(higher_cards.Bottom(), lho_suit.Top())) {
            AddReversedCards(higher_cards);
        } else {
            AddCards(higher_cards);
        }
        AddReversedCards(playable_cards.Different(higher_cards));
        return;
    }

    // Ruff?
    if (trump != NOTRUMP && playable_cards.Suit(trump)) {
        if (winning_seat == Partner(seat_to_play) &&
            (trick_ending || (lho_suit && WinOver(winning_card, lho_suit.Top())))) {
            // Partner can win - don't ruff
        } else {
            Cards my_trumps = playable_cards.Suit(trump);
            if (SuitOf(winning_card) == trump) {
                if (winning_seat != Partner(seat_to_play) && WinOver(my_trumps.Top(), winning_card)) {
                    auto higher_trumps = my_trumps.Slice(my_trumps.Top(), winning_card);
                    AddReversedCards(higher_trumps);
                    playable_cards.Remove(higher_trumps);
                }
            } else if (trick_ending || lho_suit || !hands[LeftHandOpp(seat_to_play)].Suit(trump)) {
                // Lowest trump is guaranteed to win
                AddCard(my_trumps.Bottom());
                playable_cards.Remove(my_trumps.Bottom());
            } else {
                AddReversedCards(my_trumps);
                playable_cards.Remove(my_trumps);
            }
        }
    }

    // Discard
    AddReversedCards(playable_cards);
}

void ParseHand(Cards& hand, const char* str) {
    hand = Cards();
    int suit = 0;
    for (const char* p = str; *p; ++p) {
        if (*p == ' ') { ++suit; continue; }
        int rank = -1;
        for (int r = 0; r < 13; ++r) if (*p == RANK_NAMES[r][0]) { rank = r; break; }
        if (rank >= 0) hand.Add(CardOf(suit, rank));
    }
}

void PrintOrderedCards() {
    for (int i = 0; i < num_ordered; ++i) {
        if (i > 0) printf(" ");
        PrintCard(ordered_cards[i]);
    }
}

void PrintOrderedCardsQuoted() {
    printf("\"");
    for (int i = 0; i < num_ordered; ++i) {
        if (i > 0) printf(" ");
        PrintCard(ordered_cards[i]);
    }
    printf("\"");
}

int main(int argc, char* argv[]) {
    InitCards();

    // Default to quick_test_8
    const char* deal_name = "quick_test_8";
    const char* north_str = "AKQ J6 KJ 9";
    const char* west_str = "65 AK4 AQ T";
    const char* east_str = "J7 QT9 T AK";
    const char* south_str = "98 87 96 QJ";

    if (argc > 1 && strcmp(argv[1], "6") == 0) {
        deal_name = "quick_test_6";
        north_str = "KQ J6 KJ";
        west_str = "5 K4 AQ T";
        east_str = "J7 QT9";  // Note: void in diamonds, K in clubs
        south_str = "98 87 96";
    }

    ParseHand(hands[NORTH], north_str);
    ParseHand(hands[WEST], west_str);
    if (argc > 1 && strcmp(argv[1], "6") == 0) {
        // Special handling for quick_test_6 East hand with void
        hands[EAST] = Cards();
        hands[EAST].Add(CardOf(SPADE, 9));  // J
        hands[EAST].Add(CardOf(SPADE, 5));  // 7
        hands[EAST].Add(CardOf(HEART, 10)); // Q
        hands[EAST].Add(CardOf(HEART, 8));  // T
        hands[EAST].Add(CardOf(HEART, 7));  // 9
        hands[EAST].Add(CardOf(CLUB, 11));  // K
    } else {
        ParseHand(hands[EAST], east_str);
    }
    ParseHand(hands[SOUTH], south_str);

    all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);

    printf("// Reference follow ordering from C++ implementation\n");
    printf("// Deal: %s\n", deal_name);
    printf("// North: %s\n", north_str);
    printf("// West:  %s\n", west_str);
    printf("// East:  %s\n", east_str);
    printf("// South: %s\n\n", south_str);

    // Test scenarios for following
    // We'll test second seat (card_in_trick=1) and fourth seat (card_in_trick=3)
    // with different lead suits and winning situations

    struct TestCase {
        const char* name;
        int leader;          // Who led
        int lead_card;       // What was led (will determine lead_suit)
        int follower;        // Who is following
        int position;        // card_in_trick (1=2nd, 2=3rd, 3=4th)
        int winner;          // Current winning seat
        int win_card;        // Current winning card
        int strain;          // Trump suit
    };

    // For quick_test_8:
    // North: AKQ J6 KJ 9
    // West:  65 AK4 AQ T
    // East:  J7 QT9 T AK
    // South: 98 87 96 QJ

    // Quick_test_8 test cases
    TestCase tests_8[] = {
        // ============ Following suit tests ============
        // West leads spade 6, East follows (2nd seat) - East has J7, can beat 6
        {"west_leads_S6_east_follows_2nd_nt", WEST, CardOf(SPADE, 4), EAST, 1, WEST, CardOf(SPADE, 4), NOTRUMP},
        // West leads heart A, East follows (2nd seat) - can't beat A
        {"west_leads_HA_east_follows_2nd_nt", WEST, CardOf(HEART, 12), EAST, 1, WEST, CardOf(HEART, 12), NOTRUMP},
        // North leads spade A, East follows (2nd seat) - can't beat A
        {"north_leads_SA_east_follows_2nd_nt", NORTH, CardOf(SPADE, 12), EAST, 1, NORTH, CardOf(SPADE, 12), NOTRUMP},
        // West leads diamond A, South follows (4th seat), North winning with K
        {"west_leads_DA_south_follows_4th_nt", WEST, CardOf(DIAMOND, 12), SOUTH, 3, NORTH, CardOf(DIAMOND, 11), NOTRUMP},
        // West leads club T, East follows (2nd seat) - East has AK, can beat T
        {"west_leads_CT_east_follows_2nd_nt", WEST, CardOf(CLUB, 8), EAST, 1, WEST, CardOf(CLUB, 8), NOTRUMP},
        // North leads heart J, East follows (2nd seat) - East has QT9
        {"north_leads_HJ_east_follows_2nd_nt", NORTH, CardOf(HEART, 9), EAST, 1, NORTH, CardOf(HEART, 9), NOTRUMP},
        // West leads heart K, North follows (2nd seat) - North has J6
        {"west_leads_HK_north_follows_2nd_nt", WEST, CardOf(HEART, 11), NORTH, 1, WEST, CardOf(HEART, 11), NOTRUMP},

        // ============ Partner winning tests ============
        // West leads spade 5, South follows (4th seat), partner East won with J
        {"west_leads_S5_south_follows_4th_pd_wins_nt", WEST, CardOf(SPADE, 3), SOUTH, 3, EAST, CardOf(SPADE, 9), NOTRUMP},

        // ============ Ruff/discard tests (void in lead suit) ============
        // Hearts trump: West leads club, North is void - can ruff
        {"west_leads_CT_north_ruffs_hearts", WEST, CardOf(CLUB, 8), NORTH, 1, WEST, CardOf(CLUB, 8), HEART},
        // Spades trump: West leads spade, South follows with trump (has S98)
        {"west_leads_S6_south_follows_spades", WEST, CardOf(SPADE, 4), SOUTH, 1, WEST, CardOf(SPADE, 4), SPADE},
        // Hearts trump: North leads diamond K, East is void - can ruff with hearts
        {"north_leads_DK_east_ruffs_hearts", NORTH, CardOf(DIAMOND, 11), EAST, 1, NORTH, CardOf(DIAMOND, 11), HEART},

        // ============ 3rd seat tests ============
        // West leads spade, North played A, South follows 3rd seat
        {"west_leads_S6_south_follows_3rd_nt", WEST, CardOf(SPADE, 4), SOUTH, 2, NORTH, CardOf(SPADE, 12), NOTRUMP},
    };

    // Quick_test_6 test cases
    // North: KQ J6 KJ - (void in clubs)
    // West:  5 K4 AQ T
    // East:  J7 QT9 - K (void in diamonds)
    // South: 98 87 96 - (void in clubs)
    TestCase tests_6[] = {
        // ============ Following suit tests (quick_test_6) ============
        // West leads spade 5, East follows (2nd seat) - East has J7
        {"west_leads_S5_east_follows_2nd_nt", WEST, CardOf(SPADE, 3), EAST, 1, WEST, CardOf(SPADE, 3), NOTRUMP},
        // West leads heart K, North follows (2nd seat) - North has J6, can't beat K
        {"west_leads_HK_north_follows_2nd_nt", WEST, CardOf(HEART, 11), NORTH, 1, WEST, CardOf(HEART, 11), NOTRUMP},
        // North leads spade K, East follows (2nd seat) - can't beat K
        {"north_leads_SK_east_follows_2nd_nt", NORTH, CardOf(SPADE, 11), EAST, 1, NORTH, CardOf(SPADE, 11), NOTRUMP},
        // West leads diamond A, South follows (4th seat), North winning with K
        {"west_leads_DA_south_follows_4th_nt", WEST, CardOf(DIAMOND, 12), SOUTH, 3, NORTH, CardOf(DIAMOND, 11), NOTRUMP},
        // North leads heart J, East follows (2nd seat) - East has QT9
        {"north_leads_HJ_east_follows_2nd_nt", NORTH, CardOf(HEART, 9), EAST, 1, NORTH, CardOf(HEART, 9), NOTRUMP},

        // ============ Void/Discard tests (quick_test_6) ============
        // NT: West leads diamond, East is void - must discard (East has J7 QT9 K)
        {"west_leads_DA_east_discards_nt", WEST, CardOf(DIAMOND, 12), EAST, 1, WEST, CardOf(DIAMOND, 12), NOTRUMP},
        // NT: West leads club, North is void - must discard (North has KQ J6 KJ)
        {"west_leads_CT_north_discards_nt", WEST, CardOf(CLUB, 8), NORTH, 1, WEST, CardOf(CLUB, 8), NOTRUMP},
        // NT: West leads club, South is void - must discard (South has 98 87 96)
        {"west_leads_CT_south_discards_nt", WEST, CardOf(CLUB, 8), SOUTH, 1, WEST, CardOf(CLUB, 8), NOTRUMP},

        // ============ Ruff tests with voids (quick_test_6) ============
        // Hearts trump: West leads diamond, East void - ruff with hearts
        {"west_leads_DA_east_ruffs_hearts", WEST, CardOf(DIAMOND, 12), EAST, 1, WEST, CardOf(DIAMOND, 12), HEART},
        // Spades trump: West leads club, North void - can ruff with spades (North has KQ in spades)
        {"west_leads_CT_north_ruffs_spades", WEST, CardOf(CLUB, 8), NORTH, 1, WEST, CardOf(CLUB, 8), SPADE},
        // Diamonds trump: West leads club, South void - can ruff with diamonds (South has 96)
        {"west_leads_CT_south_ruffs_diamonds", WEST, CardOf(CLUB, 8), SOUTH, 1, WEST, CardOf(CLUB, 8), DIAMOND},

        // ============ 3rd seat tests (quick_test_6) ============
        // West leads spade, North played K, South follows 3rd seat
        {"west_leads_S5_south_follows_3rd_nt", WEST, CardOf(SPADE, 3), SOUTH, 2, NORTH, CardOf(SPADE, 11), NOTRUMP},
    };

    TestCase* tests;
    int num_tests;
    const char* hands_func;

    bool use_test_6 = (argc > 1 && strcmp(argv[1], "6") == 0);
    if (use_test_6) {
        tests = tests_6;
        num_tests = sizeof(tests_6) / sizeof(tests_6[0]);
        hands_func = "get_quick_test_6_hands";
    } else {
        tests = tests_8;
        num_tests = sizeof(tests_8) / sizeof(tests_8[0]);
        hands_func = "get_quick_test_8_hands";
    }

    for (int i = 0; i < num_tests; ++i) {
        TestCase& t = tests[i];

        seat_to_play = t.follower;
        lead_suit = SuitOf(t.lead_card);
        winning_seat = t.winner;
        winning_card = t.win_card;
        card_in_trick = t.position;
        trump = t.strain;

        Reset();
        Cards playable = GetPlayableCards(t.follower, lead_suit);
        Follow(playable);

        printf("// Test: %s\n", t.name);
        printf("// Leader: %s, Lead: ", SEAT_NAMES[t.leader]);
        PrintCard(t.lead_card);
        printf(", Follower: %s (pos %d), Winner: %s with ",
               SEAT_NAMES[t.follower], t.position, SEAT_NAMES[t.winner]);
        PrintCard(t.win_card);
        printf(", Trump: %s\n", STRAIN_NAMES[t.strain]);
        printf("// %s follows with: ", SEAT_NAMES[t.follower]);
        PrintOrderedCards();
        printf("\n\n");
    }

    // Now generate Rust test functions
    printf("\n// ============== Rust test functions ==============\n\n");

    for (int i = 0; i < num_tests; ++i) {
        TestCase& t = tests[i];

        seat_to_play = t.follower;
        lead_suit = SuitOf(t.lead_card);
        winning_seat = t.winner;
        winning_card = t.win_card;
        card_in_trick = t.position;
        trump = t.strain;

        Reset();
        Cards playable = GetPlayableCards(t.follower, lead_suit);
        Follow(playable);

        printf("#[test]\n");
        printf("fn test_follow_%s() {\n", t.name);
        printf("    let hands = %s();\n", hands_func);
        printf("    let lead_suit = %s;\n", SUIT_NAMES[lead_suit]);
        printf("    let playable = get_playable(&hands, %s, lead_suit);\n", SEAT_NAMES_UPPER[t.follower]);
        printf("    let ordered = order_follows(\n");
        printf("        playable,\n");
        printf("        &hands,\n");
        printf("        %s,\n", SEAT_NAMES_UPPER[t.follower]);
        printf("        %s,\n", STRAIN_CONSTS[t.strain]);
        printf("        lead_suit,\n");
        printf("        %s,  // winning_seat\n", SEAT_NAMES_UPPER[t.winner]);
        printf("        card_of(%s, %d),  // winning_card: ", SUIT_NAMES[SuitOf(t.win_card)], RankOf(t.win_card));
        PrintCard(t.win_card);
        printf("\n");
        printf("        %d,  // card_in_trick\n", t.position);
        printf("        |c1, c2| wins_over(c1, c2, %s),\n", STRAIN_CONSTS[t.strain]);
        printf("    );\n");
        printf("    let result = ordered_to_string(ordered.iter());\n");
        printf("    assert_eq!(result, ");
        PrintOrderedCardsQuoted();
        printf(");\n");
        printf("}\n\n");
    }

    return 0;
}
