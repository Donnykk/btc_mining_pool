<template>
    <div class="home-container">
        <!-- 添加动态粒子背景 -->
        <div class="particles">
            <div v-for="n in 30" :key="n" class="particle" :style="{
                left: `${Math.random() * 100}%`,
                animationDelay: `${Math.random() * 5}s`,
                animationDuration: `${10 + Math.random() * 10}s`
            }"></div>
        </div>

        <div class="hero-section">
            <div class="bitcoin-logo">₿</div>
            <h1 class="hero-title">开始挖矿之旅</h1>
            <p class="hero-subtitle">加入我们的矿池，开始您的加密货币挖矿之旅</p>
            <div class="hero-actions">
                <button class="btn btn-primary" @click="showLoginModal">
                    <span class="btn-icon">🚀</span>
                    立即开始
                </button>
                <button class="btn btn-secondary" @click="learnMore">
                    <span class="btn-icon">📖</span>
                    了解更多
                </button>
            </div>
        </div>

        <div class="features-section">
            <div class="feature-card">
                <div class="feature-icon">⚡</div>
                <h3>高效挖矿</h3>
                <p>优化的矿池策略，提供最大化的挖矿收益</p>
                <div class="feature-glow"></div>
            </div>
            <div class="feature-card">
                <div class="feature-icon">🔒</div>
                <h3>安全可靠</h3>
                <p>先进的安全措施，保护您的挖矿收益</p>
                <div class="feature-glow"></div>
            </div>
            <div class="feature-card">
                <div class="feature-icon">📊</div>
                <h3>实时监控</h3>
                <p>随时查看您的矿机状态和收益数据</p>
                <div class="feature-glow"></div>
            </div>
        </div>
    </div>
</template>

<style scoped>
/* 现有样式保持不变，添加新样式 */

/* 粒子动画背景 */
.particles {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
    z-index: 0;
}

.particle {
    position: absolute;
    width: 4px;
    height: 4px;
    background: #4299e1;
    border-radius: 50%;
    animation: moveUp linear infinite;
}

@keyframes moveUp {
    0% {
        transform: translateY(100vh) scale(0);
        opacity: 0;
    }
    50% {
        opacity: 0.5;
    }
    100% {
        transform: translateY(-100px) scale(1);
        opacity: 0;
    }
}

/* 比特币 Logo */
.bitcoin-logo {
    font-size: 6rem;
    color: #f7931a;
    margin-bottom: 2rem;
    animation: float 6s ease-in-out infinite;
    text-shadow: 0 0 20px rgba(247, 147, 26, 0.5);
}

@keyframes float {
    0%, 100% {
        transform: translateY(0);
    }
    50% {
        transform: translateY(-20px);
    }
}

/* 标题增强 */
.hero-title {
    background: linear-gradient(45deg, #63b3ed, #f7931a);
    -webkit-background-clip: text;
    background-clip: text;
    color: transparent;
    text-shadow: 0 0 20px rgba(99, 179, 237, 0.3);
}

/* 按钮样式增强 */
.btn {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    position: relative;
    overflow: hidden;
}

.btn::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(
        90deg,
        transparent,
        rgba(255, 255, 255, 0.2),
        transparent
    );
    transition: 0.5s;
}

.btn:hover::before {
    left: 100%;
}

.btn-primary {
    background: linear-gradient(45deg, #4299e1, #3182ce);
    box-shadow: 0 0 15px rgba(66, 153, 225, 0.3);
}

.btn-secondary {
    border: 2px solid transparent;
    background: linear-gradient(#1a202c, #1a202c) padding-box,
                linear-gradient(45deg, #4299e1, #f7931a) border-box;
}

/* 特性卡片增强 */
.feature-card {
    position: relative;
    background: linear-gradient(145deg, #1a202c, #2d3748);
    border: 1px solid rgba(66, 153, 225, 0.1);
    box-shadow: 0 0 20px rgba(0, 0, 0, 0.2);
    overflow: hidden;
}

.feature-glow {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: radial-gradient(
        circle at 50% 50%,
        rgba(66, 153, 225, 0.1),
        transparent 70%
    );
    opacity: 0;
    transition: opacity 0.3s ease;
}

.feature-card:hover .feature-glow {
    opacity: 1;
}

.feature-icon {
    position: relative;
    z-index: 1;
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0% {
        transform: scale(1);
    }
    50% {
        transform: scale(1.1);
    }
    100% {
        transform: scale(1);
    }
}

/* 响应式调整 */
@media (max-width: 768px) {
    .features-section {
        grid-template-columns: 1fr;
    }
    
    .hero-title {
        font-size: 2.5rem;
    }
    
    .bitcoin-logo {
        font-size: 4rem;
    }
}
</style>

<script>
import { inject } from 'vue';

export default {
    setup() {
        const showLoginModal = inject('showLoginModal');

        const learnMore = () => {
            window.scrollTo({
                top: document.querySelector('.features-section').offsetTop,
                behavior: 'smooth'
            });
        };

        return {
            showLoginModal,
            learnMore,
        };
    },
};
</script>

<style scoped>
.home-container {
    min-height: 100vh;
    background-color: #1a202c;
    color: #fff;
    padding: 2rem;
}

.hero-section {
    text-align: center;
    padding: 4rem 2rem;
    max-width: 800px;
    margin: 0 auto;
}

.hero-title {
    font-size: 3rem;
    font-weight: bold;
    color: #63b3ed;
    margin-bottom: 1rem;
}

.hero-subtitle {
    font-size: 1.5rem;
    color: #a0aec0;
    margin-bottom: 2rem;
}

.hero-actions {
    display: flex;
    gap: 1rem;
    justify-content: center;
}

.btn {
    padding: 0.5rem 1rem;
    border: none;
    border-radius: 0.25rem;
    cursor: pointer;
    transition: background-color 0.3s;
    color: #fff;
}

.btn-primary {
    background-color: #4299e1;
    padding: 0.75rem 2rem;
    font-size: 1.1rem;
}

.btn-primary:hover {
    background-color: #3182ce;
}

.btn-secondary {
    background-color: transparent;
    border: 2px solid #4299e1;
    padding: 0.75rem 2rem;
    font-size: 1.1rem;
}

.btn-secondary:hover {
    background-color: rgba(66, 153, 225, 0.1);
}

.features-section {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 2rem;
    padding: 4rem 2rem;
    background-color: #2d3748;
    border-radius: 1rem;
    margin: 2rem auto;
    max-width: 1200px;
}

.feature-card {
    text-align: center;
    padding: 2rem;
    background-color: #1a202c;
    border-radius: 0.5rem;
    transition: transform 0.3s ease;
}

.feature-card:hover {
    transform: translateY(-10px);
}

.feature-icon {
    font-size: 3rem;
    margin-bottom: 1rem;
}

.feature-card h3 {
    font-size: 1.5rem;
    color: #63b3ed;
    margin-bottom: 1rem;
}

.feature-card p {
    color: #a0aec0;
    line-height: 1.5;
}
</style>