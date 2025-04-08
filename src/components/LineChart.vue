<template>
  <div ref="chartContainer" class="chart-container"></div>
</template>

<script>
import { ref, onMounted, watch } from 'vue'
import * as echarts from 'echarts'

export default {
  name: 'LineChart',
  props: {
    data: {
      type: Array,
      required: true
    }
  },
  setup(props) {
    const chartContainer = ref(null)
    let chart = null

    const initChart = () => {
      if (!chartContainer.value) return
      
      chart = echarts.init(chartContainer.value)
      
      const option = {
        grid: {
          top: 40,
          right: 20,
          bottom: 40,
          left: 60,
          containLabel: true
        },
        tooltip: {
          trigger: 'axis'
        },
        xAxis: {
          type: 'category',
          data: props.data.map(item => item.label),
          axisLine: {
            lineStyle: {
              color: '#a0aec0'
            }
          }
        },
        yAxis: {
          type: 'value',
          axisLine: {
            lineStyle: {
              color: '#a0aec0'
            }
          },
          splitLine: {
            lineStyle: {
              color: 'rgba(160, 174, 192, 0.1)'
            }
          }
        },
        series: [{
          data: props.data.map(item => item.value),
          type: 'line',
          smooth: true,
          symbolSize: 8,
          lineStyle: {
            width: 3,
            color: '#4299e1'
          },
          itemStyle: {
            color: '#4299e1'
          },
          areaStyle: {
            color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
              {
                offset: 0,
                color: 'rgba(66, 153, 225, 0.3)'
              },
              {
                offset: 1,
                color: 'rgba(66, 153, 225, 0.1)'
              }
            ])
          }
        }]
      }
      
      chart.setOption(option)
    }

    watch(() => props.data, () => {
      initChart()
    }, { deep: true })

    onMounted(() => {
      initChart()
      window.addEventListener('resize', () => {
        chart && chart.resize()
      })
    })

    return {
      chartContainer
    }
  }
}
</script>

<style scoped>
.chart-container {
  width: 100%;
  height: 100%;  /* Take full height of parent */
}
</style>