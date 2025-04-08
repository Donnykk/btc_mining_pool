import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/Home.vue'
import DashboardView from '../views/Dashboard.vue'
import MainLayout from '../layouts/MainLayout.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      component: MainLayout,
      children: [
        {
          path: '',
          name: 'home',
          component: HomeView
        },
        {
          path: 'dashboard',
          name: 'dashboard',
          component: DashboardView
        }
      ]
    }
  ]
})

export default router
