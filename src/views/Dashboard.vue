<template>
  <div class="dashboard">
    <!-- 添加动态网格背景 -->
    <div class="grid-background"></div>

    <h1 class="dashboard-title">
      <span class="bitcoin-symbol">₿</span> Mining Dashboard
      <div class="title-underline"></div>
    </h1>

    <div class="stats-container">
      <div class="stat-card">
        <div class="stat-icon">
          <i class="fas fa-microchip"></i>
          <div class="icon-pulse"></div>
        </div>
        <div class="stat-content">
          <h2 class="stat-title">Hashrate</h2>
          <p class="stat-value">{{ hashrate }} TH/s</p>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">
          <i class="fas fa-coins"></i>
          <div class="icon-pulse"></div>
        </div>
        <div class="stat-content">
          <h2 class="stat-title">Total Earnings</h2>
          <p class="stat-value">{{ earnings }} BTC</p>
        </div>
      </div>
    </div>

    <div class="chart-card">
      <h2 class="chart-title">
        Hashrate Trend
        <div class="title-underline"></div>
      </h2>
      <LineChart :data="chartData" />
    </div>
  </div>
</template>

<style scoped>
/* 现有样式保持不变，添加新样式 */

/* 动态网格背景 */
.grid-background {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-image: 
    linear-gradient(rgba(66, 153, 225, 0.1) 1px, transparent 1px),
    linear-gradient(90deg, rgba(66, 153, 225, 0.1) 1px, transparent 1px);
  background-size: 20px 20px;
  pointer-events: none;
  animation: gridMove 15s linear infinite;
}

@keyframes gridMove {
  0% {
    transform: translateY(0);
  }
  100% {
    transform: translateY(20px);
  }
}

/* 比特币符号 */
.bitcoin-symbol {
  color: #f7931a;
  margin-right: 0.5rem;
}

/* 标题下划线 */
.title-underline {
  width: 100px;
  height: 3px;
  background: linear-gradient(90deg, #4299e1, #f7931a);
  margin: 0.5rem auto;
  border-radius: 2px;
}

/* 统计卡片样式增强 */
.stat-card {
  background: linear-gradient(145deg, #2d3748, #1a202c);
  border: 1px solid rgba(66, 153, 225, 0.2);
  position: relative;
  overflow: hidden;
}

.stat-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: radial-gradient(circle at 50% 50%, rgba(66, 153, 225, 0.1), transparent 70%);
  opacity: 0;
  transition: opacity 0.3s ease;
}

.stat-card:hover::before {
  opacity: 1;
}

/* 图标脉冲效果 */
.icon-pulse {
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: rgba(66, 153, 225, 0.2);
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0% {
    transform: scale(0.95);
    opacity: 0.5;
  }
  50% {
    transform: scale(1.2);
    opacity: 0;
  }
  100% {
    transform: scale(0.95);
    opacity: 0.5;
  }
}

/* 图表卡片样式增强 */
.chart-card {
  background: linear-gradient(145deg, #2d3748, #1a202c);
  border: 1px solid rgba(66, 153, 225, 0.2);
  position: relative;
  overflow: hidden;
}

.chart-card::after {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 2px;
  background: linear-gradient(90deg, transparent, #4299e1, transparent);
  animation: scanline 3s linear infinite;
}

@keyframes scanline {
  0% {
    left: -100%;
  }
  100% {
    left: 100%;
  }
}

/* 数值动画效果 */
.stat-value {
  background: linear-gradient(45deg, #fff, #63b3ed);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
  position: relative;
}

/* 响应式调整 */
@media (max-width: 768px) {
  .stats-container {
    grid-template-columns: 1fr;
  }
  
  .dashboard-title {
    font-size: 1.75rem;
  }
}
</style>

<script>
import { ref, onMounted } from "vue";
import LineChart from "@/components/LineChart.vue";

export default {
  name: "Dashboard",
  components: {
    LineChart,
  },
  setup() {
    const hashrate = ref(123.45);
    const earnings = ref(0.0123);
    const chartData = ref([
      { label: "Day1", value: 120 },
      { label: "Day2", value: 130 },
      { label: "Day3", value: 150 },
    ]);

    const fetchStats = async () => {
      // TODO: 在此处异步请求真实数据
      // hashrate.value = ...
      // earnings.value = ...
      // chartData.value = ...
    };

    onMounted(fetchStats);

    return {
      hashrate,
      earnings,
      chartData,
    };
  },
};
</script>

<style scoped>
/* 整个 Dashboard 区域 */
.dashboard {
  /* 背景色、文本色 */
  background-color: #1a202c;
  color: #fff;
  
  /* 渐变背景 */
  background-image: linear-gradient(to bottom, #1a202c, #2d3748);
  
  /* 占满视窗高度，让页面撑满 */
  min-height: 100vh;

  /* 内边距 */
  padding: 2rem;
}

/* Dashboard 标题 */
.dashboard-title {
  font-size: 2.25rem;
  font-weight: 700;
  margin-bottom: 2rem;
  text-align: center;
  color: #90cdf4;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
  letter-spacing: 1px;
}

/* 统计卡片容器：使用网格布局，也可用 flex 布局 */
.stats-container {
  display: grid;
  grid-template-columns: 1fr 1fr;
  /* 两列 */
  gap: 2rem;
  /* 卡片之间的间距 */
  margin-bottom: 2.5rem;
  /* 与后面内容分开 */
}

/* 单个统计卡片 */
.stat-card {
  background-color: #2d3748;
  padding: 1.75rem;
  border-radius: 1rem;
  box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3);
  transition: transform 0.3s ease, box-shadow 0.3s ease;
  display: flex;
  align-items: center;
  border-left: 4px solid #4299e1;
}

.stat-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 15px 20px -3px rgba(0, 0, 0, 0.4);
}

/* 统计卡片图标 */
.stat-icon {
  font-size: 2.5rem;
  color: #4299e1;
  margin-right: 1.5rem;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 60px;
  height: 60px;
  background-color: rgba(66, 153, 225, 0.1);
  border-radius: 50%;
}

/* 统计卡片内容区 */
.stat-content {
  flex: 1;
}

/* 卡片里的标题 */
.stat-title {
  font-size: 1.25rem;
  font-weight: 600;
  margin-bottom: 0.5rem;
  color: #a0aec0;
}

/* 统计值 */
.stat-value {
  font-size: 2rem;
  font-weight: 700;
  margin-top: 0.5rem;
  color: #fff;
}

/* 图表卡片 */
.chart-card {
  background-color: #2d3748;
  padding: 2rem;
  border-radius: 1rem;
  box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3);
  border-top: 4px solid #4299e1;
  height: 400px; /* Add fixed height */
}

/* 图表区域标题 */
.chart-title {
  font-size: 1.5rem;
  font-weight: 600;
  margin-bottom: 1.5rem;
  color: #a0aec0;
  text-align: center;
}
</style>