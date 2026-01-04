// Compile: g++ -O0 -std=c++17 -o gen_lead_order_refs gen_lead_order_refs.cc
// Generate reference lead ordering for all seats and strains

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
};

Cards hands[4];
Cards all_cards;
int trump = NOTRUMP;
int seat_to_play = WEST;

int Partner(int seat) { return (seat + 2) & 3; }
int LeftHandOpp(int seat) { return (seat + 1) & 3; }
int RightHandOpp(int seat) { return (seat + 3) & 3; }

char ordered_cards[52];
int num_ordered = 0;

void Reset() { num_ordered = 0; }
void AddCard(int card) { ordered_cards[num_ordered++] = card; }
void AddCards(Cards cards) { for (int c : cards) AddCard(c); }

template<bool SUIT_CONTRACT>
void Lead(Cards playable_cards) {
    Cards good_leads, high_leads, leads, bad_leads, trump_leads, ruff_leads;
    auto pd_hand = hands[Partner(seat_to_play)];
    auto lho_hand = hands[LeftHandOpp(seat_to_play)];
    auto rho_hand = hands[RightHandOpp(seat_to_play)];

    for (int suit = 0; suit < NUM_SUITS; ++suit) {
        auto my_suit = playable_cards.Suit(suit);
        if (!my_suit) continue;
        if (SUIT_CONTRACT) {
            if (suit == trump) {
                trump_leads.Add(my_suit.Top());
                trump_leads.Add(my_suit.Bottom());
                continue;
            }
            if (lho_hand.Suit(trump) && !lho_hand.Suit(suit)) continue;
            if (rho_hand.Suit(trump) && !rho_hand.Suit(suit)) continue;
        }
        auto pd_suit = pd_hand.Suit(suit), our_suits = my_suit.Union(pd_suit);
        auto lho_suit = lho_hand.Suit(suit);
        auto all_suit_cards = all_cards.Suit(suit);
        int a = all_suit_cards.Top();
        int k = all_suit_cards.Remove(a).Top();
        int q = all_suit_cards.Remove(k).Top();
        int j = all_suit_cards.Remove(q).Top();
        int t = all_suit_cards.Remove(j).Top();

        if (pd_suit.Size() >= 2 && lho_suit.Size() >= 2) {
            if ((pd_suit.Have(k) && lho_suit.Have(a)) ||
                (pd_suit.Have(a) && lho_suit.Have(k) &&
                 (pd_suit.Have(q) || our_suits.Include(Cards().Add(q).Add(j)))) ||
                (pd_suit.Have(k) && lho_suit.Have(q) &&
                 (pd_suit.Have(j) || our_suits.Include(Cards().Add(j).Add(t))))) {
                good_leads.Add(my_suit.Top());
                good_leads.Add(my_suit.Bottom());
                continue;
            }
        }
        auto rho_suit = rho_hand.Suit(suit);
        auto partnership_cards = hands[seat_to_play].Union(pd_hand);
        if (my_suit.Size() >= 2 && rho_suit.Size() >= 2) {
            if ((my_suit.Have(a) && rho_suit.Have(k)) ||
                (my_suit.Have(k) && rho_suit.Have(a) && !partnership_cards.Have(q))) {
                if (SUIT_CONTRACT) {
                    bad_leads.Add(my_suit.Top());
                    bad_leads.Add(my_suit.Bottom());
                }
                continue;
            }
        }
        Cards akq = Cards().Add(a).Add(k).Add(q);
        if (lho_suit && rho_suit && partnership_cards.Intersect(akq).Size() >= 2) {
            high_leads.Add(my_suit.Top());
            high_leads.Add(my_suit.Bottom());
            continue;
        }
        if (SUIT_CONTRACT && !pd_suit && lho_suit && rho_suit && pd_hand.Suit(trump) &&
            pd_hand.Suit(trump).Size() <= playable_cards.Suit(trump).Size() &&
            my_suit.Bottom() != a) {
            ruff_leads.Add(my_suit.Bottom());
            continue;
        }
        leads.Add(my_suit.Top());
        leads.Add(my_suit.Bottom());
    }
    if (SUIT_CONTRACT) {
        AddCards(ruff_leads);
        playable_cards.Remove(ruff_leads);
    }
    AddCards(good_leads);
    playable_cards.Remove(good_leads);
    AddCards(high_leads);
    playable_cards.Remove(high_leads);
    AddCards(leads);
    playable_cards.Remove(leads);
    if (SUIT_CONTRACT) {
        AddCards(bad_leads);
        playable_cards.Remove(bad_leads);
        AddCards(trump_leads);
        playable_cards.Remove(trump_leads);
    }
    AddCards(playable_cards);
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

    // Check for quick_test_6 argument
    if (argc > 1 && strcmp(argv[1], "6") == 0) {
        deal_name = "quick_test_6";
        north_str = "KQ J6 KJ";
        west_str = "5 K4 AQ T";
        east_str = "J7 QT9  K";
        south_str = "98 87 96";
    }

    ParseHand(hands[NORTH], north_str);
    ParseHand(hands[WEST], west_str);
    ParseHand(hands[EAST], east_str);
    ParseHand(hands[SOUTH], south_str);

    all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);

    // Generate Rust test code
    printf("// Reference lead ordering from C++ implementation\n");
    printf("// Deal: %s\n", deal_name);
    printf("// North: %s\n", north_str);
    printf("// West:  %s\n", west_str);
    printf("// East:  %s\n", east_str);
    printf("// South: %s\n\n", south_str);

    // Generate for all seats and strains
    int strains[] = {NOTRUMP, SPADE, HEART, DIAMOND, CLUB};

    for (int strain_idx = 0; strain_idx < 5; ++strain_idx) {
        trump = strains[strain_idx];
        printf("// Trump: %s\n", STRAIN_NAMES[trump]);

        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            Reset();

            if (trump == NOTRUMP) {
                Lead<false>(hands[seat]);
            } else {
                Lead<true>(hands[seat]);
            }

            printf("// %s leads: ", SEAT_NAMES[seat]);
            PrintOrderedCards();
            printf("\n");
        }
        printf("\n");
    }

    // Now generate Rust test functions
    printf("\n// ============== Rust test functions ==============\n\n");

    for (int strain_idx = 0; strain_idx < 5; ++strain_idx) {
        trump = strains[strain_idx];

        for (int seat = 0; seat < 4; ++seat) {
            seat_to_play = seat;
            Reset();

            if (trump == NOTRUMP) {
                Lead<false>(hands[seat]);
            } else {
                Lead<true>(hands[seat]);
            }

            printf("#[test]\n");
            printf("fn test_%s_%s_%s_leads() {\n", deal_name, STRAIN_NAMES_LOWER[trump], SEAT_NAMES_LOWER[seat]);
            printf("    let hands = get_%s_hands();\n", deal_name);
            printf("    let all_cards = hands.all_cards();\n");
            printf("    let ordered = order_leads(hands[%s], &hands, %s, %s, all_cards);\n",
                   SEAT_NAMES_UPPER[seat], SEAT_NAMES_UPPER[seat], STRAIN_CONSTS[trump]);
            printf("    let result = ordered_to_string(ordered.iter());\n");
            printf("    assert_eq!(result, ");
            PrintOrderedCardsQuoted();
            printf(");\n");
            printf("}\n\n");
        }
    }

    return 0;
}
