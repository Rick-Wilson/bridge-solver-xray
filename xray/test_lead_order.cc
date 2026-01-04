// Compile: g++ -O0 -std=c++17 -o test_lead_order test_lead_order.cc
// Test lead ordering with correct card encoding

#include <cstdio>
#include <cstdint>

static constexpr int NUM_SUITS = 4;
static constexpr int NUM_RANKS = 13;
static constexpr int TOTAL_CARDS = 52;
static constexpr int NOTRUMP = 4;
static constexpr int WEST = 0, NORTH = 1, EAST = 2, SOUTH = 3;

// Card encoding: card = suit * 13 + (12 - rank), where Ace=12
int suit_of[TOTAL_CARDS];
int rank_of[TOTAL_CARDS];
int card_of[NUM_SUITS][NUM_RANKS];

int SuitOf(int card) { return suit_of[card]; }
int RankOf(int card) { return rank_of[card]; }
int CardOf(int suit, int rank) { return card_of[suit][rank]; }
uint64_t MaskOf(int suit) { return 0x1fffULL << (suit * NUM_RANKS); }

const char* SUIT_NAMES[] = {"S", "H", "D", "C"};
const char* RANK_NAMES[] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};

void InitCards() {
    for (int card = 0; card < TOTAL_CARDS; ++card) {
        suit_of[card] = card / NUM_RANKS;
        rank_of[card] = NUM_RANKS - 1 - card % NUM_RANKS;
        card_of[suit_of[card]][rank_of[card]] = card;
    }
}

void PrintCard(int card) {
    printf("%s%s", RANK_NAMES[RankOf(card)], SUIT_NAMES[SuitOf(card)]);
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
        
        printf("  Suit %s: a=", SUIT_NAMES[suit]); PrintCard(a);
        printf(" k="); PrintCard(k);
        printf(" q="); PrintCard(q);
        printf(" j="); PrintCard(j);
        printf(" t="); PrintCard(t);
        printf("\n");
        printf("    my_suit: "); for (int c : my_suit) { PrintCard(c); printf(" "); } printf("\n");
        printf("    pd_suit: "); for (int c : pd_suit) { PrintCard(c); printf(" "); } printf("\n");
        printf("    lho_suit: "); for (int c : lho_suit) { PrintCard(c); printf(" "); } printf("\n");
        printf("    our_suits.Have(Cards().Add(q).Add(j)) = %d\n", our_suits.Have(Cards().Add(q).Add(j)));
        printf("    our_suits.Include(Cards().Add(q).Add(j)) = %d\n", our_suits.Include(Cards().Add(q).Add(j)));
        
        if (pd_suit.Size() >= 2 && lho_suit.Size() >= 2) {
            if ((pd_suit.Have(k) && lho_suit.Have(a)) ||
                (pd_suit.Have(a) && lho_suit.Have(k) &&
                 (pd_suit.Have(q) || our_suits.Have(Cards().Add(q).Add(j)))) ||
                (pd_suit.Have(k) && lho_suit.Have(q) &&
                 (pd_suit.Have(j) || our_suits.Have(Cards().Add(j).Add(t))))) {
                printf("    -> GOOD LEAD\n");
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
                printf("    -> BAD LEAD (skipped in NT)\n");
                if (SUIT_CONTRACT) {
                    bad_leads.Add(my_suit.Top());
                    bad_leads.Add(my_suit.Bottom());
                }
                continue;
            }
        }
        Cards akq = Cards().Add(a).Add(k).Add(q);
        if (lho_suit && rho_suit && partnership_cards.Intersect(akq).Size() >= 2) {
            printf("    -> HIGH LEAD\n");
            high_leads.Add(my_suit.Top());
            high_leads.Add(my_suit.Bottom());
            continue;
        }
        if (SUIT_CONTRACT && !pd_suit && lho_suit && rho_suit && pd_hand.Suit(trump) &&
            pd_hand.Suit(trump).Size() <= playable_cards.Suit(trump).Size() &&
            my_suit.Bottom() != a) {
            printf("    -> RUFF LEAD\n");
            ruff_leads.Add(my_suit.Bottom());
            continue;
        }
        printf("    -> NORMAL LEAD\n");
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

int main() {
    InitCards();
    
    // North: AKQ J6 KJ 9
    // West:  65 AK4 AQ T
    // East:  J7 QT9 T AK
    // South: 98 87 96 QJ
    
    ParseHand(hands[NORTH], "AKQ J6 KJ 9");
    ParseHand(hands[WEST], "65 AK4 AQ T");
    ParseHand(hands[EAST], "J7 QT9 T AK");
    ParseHand(hands[SOUTH], "98 87 96 QJ");
    
    all_cards = hands[0].Union(hands[1]).Union(hands[2]).Union(hands[3]);
    
    printf("=== Test: West leads in NT ===\n");
    printf("West hand: ");
    for (int c : hands[WEST]) { PrintCard(c); printf(" "); }
    printf("\n\n");
    
    printf("Partner (East): ");
    for (int c : hands[Partner(WEST)]) { PrintCard(c); printf(" "); }
    printf("\nLHO (North): ");
    for (int c : hands[LeftHandOpp(WEST)]) { PrintCard(c); printf(" "); }
    printf("\nRHO (South): ");
    for (int c : hands[RightHandOpp(WEST)]) { PrintCard(c); printf(" "); }
    printf("\n\n");
    
    seat_to_play = WEST;
    trump = NOTRUMP;
    
    Reset();
    Lead<false>(hands[WEST]);
    
    printf("\nOrdered cards: ");
    for (int i = 0; i < num_ordered; ++i) {
        PrintCard(ordered_cards[i]);
        printf(" ");
    }
    printf("\n");
    
    return 0;
}
