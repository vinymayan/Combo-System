#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Utils
{
    class DelayedDispatcher
    {
    public:
        using Task = std::move_only_function<void()>;
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        static DelayedDispatcher& Get()
        {
            static DelayedDispatcher instance;
            return instance;
        }

        template <class Rep, class Period>
        void PostDelayed(std::chrono::duration<Rep, Period> delay, Task&& task)
        {
            const auto executeAt = Clock::now() + delay;

            {
                std::scoped_lock lock(m_mutex);
                m_queue.emplace(executeAt, std::move(task));
            }
            m_cv.notify_one();
        }

        // NOVO: Congela o processamento de tarefas e marca o início da pausa
        void Pause()
        {
            std::unique_lock lock(m_mutex);
            if (!m_isPaused) {
                m_isPaused = true;
                m_pauseTime = Clock::now();
                m_cv.notify_all(); // Acorda a thread para ela entrar em estado de espera
            }
        }

        // NOVO: Calcula a duração da pausa e posterga todas as threads agendadas
        void Resume()
        {
            std::unique_lock lock(m_mutex);
            if (m_isPaused) {
                auto pauseDuration = Clock::now() - m_pauseTime;

                // Como std::priority_queue não permite alteração interna direta, extraímos os dados temporariamente
                std::vector<ScheduledTask> tempTasks;
                while (!m_queue.empty()) {
                    ScheduledTask task = std::move(const_cast<ScheduledTask&>(m_queue.top()));
                    m_queue.pop();

                    // Adiciona o tempo que o jogo ficou congelado na tarefa
                    task.time += pauseDuration;
                    tempTasks.push_back(std::move(task));
                }

                // Devolve as tarefas ajustadas com segurança para a fila
                for (auto& task : tempTasks) {
                    m_queue.push(std::move(task));
                }

                m_isPaused = false;
                m_cv.notify_all(); // Retoma o fluxo de monitoramento temporal
            }
        }

        void Stop()
        {
            m_worker.request_stop();
        }

    private:
        struct ScheduledTask
        {
            TimePoint time;
            mutable Task task;

            bool operator>(const ScheduledTask& other) const {
                return time > other.time;
            }
        };

        DelayedDispatcher()
        {
            m_worker = std::jthread([this](std::stop_token stoken) {
                RunLoop(std::move(stoken));
                });
        }

        ~DelayedDispatcher()
        {
            Stop();
        }

        void RunLoop(std::stop_token stoken)
        {
            while (!stoken.stop_requested()) {
                Task task_to_run;

                {
                    std::unique_lock lock(m_mutex);

                    // Condicional expandida para acordar também em caso de alteração no estado de pausa
                    m_cv.wait(lock, stoken, [this] { return !m_queue.empty() || m_isPaused; });

                    if (stoken.stop_requested()) {
                        return;
                    }

                    // Se estiver pausado, bloqueia o loop aqui até o método Resume() ser invocado
                    if (m_isPaused) {
                        m_cv.wait(lock, stoken, [this] { return !m_isPaused; });
                        if (stoken.stop_requested()) return;
                        continue;
                    }

                    auto now = Clock::now();
                    auto& top_task = m_queue.top();

                    if (top_task.time <= now) {
                        task_to_run = std::move(top_task.task);
                        m_queue.pop();
                    }
                    else {
                        auto sleep_until = top_task.time;
                        // Acorda mais cedo caso o jogo entre em pausa durante a soneca da thread
                        m_cv.wait_until(lock, stoken, sleep_until, [this, sleep_until] {
                            return m_isPaused || (!m_queue.empty() && m_queue.top().time < sleep_until);
                            });
                        continue;
                    }
                }

                if (task_to_run) {
                    task_to_run();
                }
            }
        }

        std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, std::greater<>> m_queue;
        std::mutex m_mutex;
        std::condition_variable_any m_cv;
        std::jthread m_worker;

        bool m_isPaused = false;
        TimePoint m_pauseTime;
    };
}