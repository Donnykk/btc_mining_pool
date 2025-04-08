<template>
    <div class="app-container">
        <!-- 添加动态背景 -->
        <div class="crypto-background">
            <div v-for="n in 20" :key="n" class="bitcoin-symbol" :style="{ 
                left: `${Math.random() * 100}%`,
                animationDelay: `${Math.random() * 5}s`,
                opacity: Math.random() * 0.3 + 0.1
            }">₿</div>
        </div>

        <nav class="navbar">
            <h1 class="logo">
                <span class="bitcoin-icon">₿</span> 
                
                <span class="tech-line"></span>
            </h1>
            <div class="nav-links">
                <router-link to="/" class="nav-link">🏠 Home</router-link>
                <router-link to="/dashboard" class="nav-link">📊 Dashboard</router-link>
                <button v-if="!isLoggedIn" @click="showLoginModal" class="btn">🔑 Login</button>
                <button v-if="isLoggedIn" @click="logout" class="btn btn-logout">🚪 Logout</button>
            </div>
        </nav>

        <div class="main-content">
            <router-view />
        </div>

        <div v-if="isLoginModalVisible" class="modal-overlay">
            <div class="modal-content">
                <h2 class="modal-title">Miner Login</h2>
                <input v-model="username" type="text" placeholder="Username" class="input-box mt-3" />
                <input v-model="password" type="password" placeholder="Password" class="input-box mt-3" />
                <div class="modal-actions">
                    <button @click="login" class="btn w-full">Login</button>
                </div>
            </div>
        </div>
    </div>
</template>

<script>
import { ref, provide } from 'vue'

export default {
    setup() {
        const isLoggedIn = ref(false)
        const isLoginModalVisible = ref(false)
        const username = ref('')
        const password = ref('')

        const showLoginModal = () => {
            isLoginModalVisible.value = true
        }

        const login = () => {
            if (username.value && password.value) {
                isLoggedIn.value = true
                isLoginModalVisible.value = false
                alert('Login Success!')
            } else {
                alert('Please enter username and password!')
            }
        }

        const logout = () => {
            isLoggedIn.value = false
            alert('Logout Success!')
        }

        // 提供 showLoginModal 方法给子组件使用
        provide('showLoginModal', showLoginModal)

        return {
            isLoggedIn,
            isLoginModalVisible,
            username,
            password,
            showLoginModal,
            login,
            logout,
        }
    }
}
</script>

<style scoped>
/* 从 Home.vue 复制相关样式 */
.app-container {
    min-height: 100vh;
    background-color: #1a202c;
    color: #fff;
    display: flex;
    flex-direction: column;
}

.navbar {
    background-color: #2d3748;
    padding: 1rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1),
        0 2px 4px -1px rgba(0, 0, 0, 0.06);
}

.logo {
    font-size: 1.5rem;
    font-weight: bold;
    color: #63b3ed;
}

.nav-links {
    display: flex;
    gap: 1rem;
}

.nav-link {
    color: #fff;
    padding: 0.5rem 1rem;
    border-radius: 0.25rem;
    text-decoration: none;
    transition: background-color 0.3s;
}

.nav-link:hover {
    background-color: #3182ce;
}

.btn {
    background-color: #4299e1;
    color: #fff;
    padding: 0.5rem 1rem;
    border: none;
    border-radius: 0.25rem;
    cursor: pointer;
    transition: background-color 0.3s;
}

.btn:hover {
    background-color: #3182ce;
}

.btn-logout {
    background-color: #f56565;
}

.main-content {
    flex: 1;
}

/* 登录模态框样式 */
.modal-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background-color: rgba(0, 0, 0, 0.5);
}

.modal-content {
    background-color: #2d3748;
    padding: 1.5rem;
    border-radius: 0.5rem;
    box-shadow: 0 10px 15px rgba(0, 0, 0, 0.5);
    width: 20rem;
}

.modal-title {
    font-size: 1.25rem;
    font-weight: 600;
    text-align: center;
    color: #fff;
}

.input-box {
    width: 100%;
    padding: 0.5rem 0.75rem;
    margin: 0;
    border: none;
    border-radius: 0.25rem;
    background-color: #4a5568;
    color: #fff;
    outline: none;
}

.input-box:focus {
    box-shadow: 0 0 0 2px #63b3ed;
}

.mt-3 {
    margin-top: 0.75rem;
}

.modal-actions {
    display: flex;
    justify-content: space-between;
    margin-top: 1rem;
}

.w-full {
    width: 100%;
}

/* 动态背景 */
.crypto-background {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    overflow: hidden;
    z-index: 0;
    pointer-events: none;
}

.bitcoin-symbol {
    position: absolute;
    font-size: 24px;
    color: rgba(247, 147, 26, 0.1); /* 比特币橙色 */
    animation: float 15s infinite linear;
}

@keyframes float {
    0% {
        transform: translateY(100vh) rotate(0deg);
    }
    100% {
        transform: translateY(-100px) rotate(360deg);
    }
}

/* Logo 样式增强 */
.logo {
    position: relative;
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.bitcoin-icon {
    color: #f7931a; /* 比特币标准色 */
    animation: pulse 2s infinite;
}

.tech-line {
    position: absolute;
    bottom: -5px;
    left: 0;
    width: 100%;
    height: 2px;
    background: linear-gradient(90deg, #4299e1, #f7931a);
    animation: scan 2s infinite;
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

@keyframes scan {
    0% {
        transform: translateX(-100%);
    }
    100% {
        transform: translateX(100%);
    }
}

/* 导航栏样式优化 */
.navbar {
    background: linear-gradient(90deg, #1a202c, #2d3748);
    border-bottom: 1px solid rgba(66, 153, 225, 0.2);
    backdrop-filter: blur(10px);
    z-index: 10;
}

.nav-link {
    position: relative;
    overflow: hidden;
}

.nav-link::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    height: 2px;
    background: linear-gradient(90deg, #4299e1, #f7931a);
    transform: translateX(-100%);
    transition: transform 0.3s ease;
}

.nav-link:hover::after {
    transform: translateX(0);
}

/* 按钮样式优化 */
.btn {
    background: linear-gradient(45deg, #4299e1, #3182ce);
    border: 1px solid rgba(66, 153, 225, 0.2);
    box-shadow: 0 0 10px rgba(66, 153, 225, 0.2);
}

.btn:hover {
    background: linear-gradient(45deg, #3182ce, #2c5282);
    box-shadow: 0 0 15px rgba(66, 153, 225, 0.3);
}

.btn-logout {
    background: linear-gradient(45deg, #f56565, #c53030);
}

.btn-logout:hover {
    background: linear-gradient(45deg, #c53030, #9b2c2c);
}

/* 模态框样式优化 */
.modal-content {
    background: linear-gradient(135deg, #2d3748, #1a202c);
    border: 1px solid rgba(66, 153, 225, 0.2);
    box-shadow: 0 0 20px rgba(0, 0, 0, 0.5);
}

.input-box {
    background: rgba(74, 85, 104, 0.8);
    border: 1px solid rgba(66, 153, 225, 0.2);
    transition: all 0.3s ease;
}

.input-box:focus {
    background: rgba(74, 85, 104, 0.9);
    box-shadow: 0 0 0 2px rgba(66, 153, 225, 0.3);
}
</style>