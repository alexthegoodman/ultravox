#include <glm/glm.hpp>

inline glm::vec2 toNDC(float x, float y, float width, float height) {
    float ndcX = (x / width) * 2.0f - 1.0f;
    float ndcY = -((y / height) * 2.0f - 1.0f);
    return { ndcX, ndcY };
}

inline glm::vec2 fromNDC(float ndcX, float ndcY, float width, float height) {
    float x = ((ndcX + 1.0f) / 2.0f) * width;
    float y = ((-ndcY + 1.0f) / 2.0f) * height;
    return { x, y };
}

inline float toSystemScale(float humanDim, float windowDim) {
    return (humanDim / windowDim) * 2.0f;
}

inline float fromSystemScale(float scaleDim, float windowDim) {
    return (scaleDim / 2.0f) * windowDim;
}
