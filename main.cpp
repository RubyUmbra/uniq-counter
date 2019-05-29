#include <set>
#include <random>
#include <cassert>

#define POW_2_32 4294967296.0

// HyperLogLog configuration
#define REGISTER_BIT_WIDTH (static_cast<uint32_t>(13))
#define REGISTER_SIZE (static_cast<uint32_t>(1) << REGISTER_BIT_WIDTH)

class UniqCounter {// HyperLogLog
    // no more than 32kb of memory should be used here

public:
    void add(int x) {
        uint32_t hash = hash_32(x);
        uint32_t index = hash >> (32 - REGISTER_BIT_WIDTH);
        uint8_t rank = _count_leading_zeros((hash << REGISTER_BIT_WIDTH));
        if (rank > register_[index]) {
            register_[index] = rank;
        }
    }

    int get_uniq_num() const {
        double sum = 0.0;
        for (uint8_t i : register_) {
            sum += 1.0 / (static_cast<uint8_t>(1) << i);
        }
        double res = ((0.7213 / (1.0 + 1.079 / REGISTER_SIZE)) * REGISTER_SIZE * REGISTER_SIZE) / sum;
        if (res <= 2.5 * REGISTER_SIZE) {
            uint32_t zeros = 0;
            for (uint8_t i : register_) {
                if (i == 0) {
                    ++zeros;
                }
            }
            if (zeros != 0) {
                res = REGISTER_SIZE * std::log(static_cast<double>(REGISTER_SIZE) / zeros);
            }
        } else if (res > (1.0 / 30.0) * POW_2_32) {
            res = -POW_2_32 * log(1.0 - (res / POW_2_32));
        }
        return static_cast<int>(res);
    }
private:
    uint8_t register_[REGISTER_SIZE]{};

    static inline uint8_t _count_leading_zeros(uint32_t x) {
        uint8_t res = 1;
        while (res <= (32 - REGISTER_BIT_WIDTH) && !(x & 0x80000000)) {
            res++;
            x <<= static_cast<uint32_t>(1);
        }
        return res;
    }

    static inline uint32_t reorder_bits(const uint32_t x, const uint8_t r) {
        return (x << r) | (x >> static_cast<uint32_t>(32 - r));
    }

    static uint32_t hash_32(const int x) {
        uint32_t q = x;
        q *= 0xcc9e2d51;
        q = reorder_bits(q, 15);
        q *= 0x1b873593;
        uint32_t res = 0x00000139;
        res ^= q;
        res = reorder_bits(res, 13);
        res = res * 5 + 0xe6546b64;
        res ^= static_cast<uint32_t>(4);
        res ^= res >> static_cast<uint32_t>(16);
        res *= 0x85ebca6b;
        res ^= res >> static_cast<uint32_t>(13);
        res *= 0xc2b2ae35;
        res ^= res >> static_cast<uint32_t>(16);
        return res;
    }
};

double relative_error(int expected, int got) {
    return abs(got - expected) / (double) expected;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());

    const int N = (int) 1e6;
    for (int k : {1, 10, 1000, 10000, N / 10, N, N * 10}) {
        std::uniform_int_distribution<> dis(1, k);
        std::set<int> all;
        UniqCounter counter;
        for (int i = 0; i < N; i++) {
            int value = dis(gen);
            all.insert(value);
            counter.add(value);
        }
        int expected = (int) all.size();
        int counter_result = counter.get_uniq_num();
        double error = relative_error(expected, counter_result);
        printf("%d numbers in range [1 .. %d], %d uniq, %d result, %.5f relative error\n", N, k, expected,
               counter_result, error);
        assert(error <= 0.1);
    }

    return 0;
}
