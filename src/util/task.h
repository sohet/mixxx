#pragma once

#include <QAtomicInt>
#include <QObject>

// Waits for a number of tasks to complete and emits allTasksDone() once all are
// complete.
// Deprecated: This is just a unnecessarily heavy counter. You should probably refrain
// from using this in new code in favor of appropriate QtConcurrent primitives.
class TaskWatcher : public QObject {
    Q_OBJECT
  public:
    TaskWatcher(QObject* pParent = nullptr)
            : QObject(pParent) {};
    ~TaskWatcher() override;

    // Increment the number of active tasks by one and watch pTask for
    // doneSignal. This should be called before the task starts to prevent a
    // race condition where the task completes before we listen for doneSignal.
    void watchTask();

    // Called when an individual task is done.
    // WARNING: Invoked in the thread the task runs in.
    void taskDone();

  signals:
    // Signaled when all watched tasks are done from the thread that emitted
    // the last taskDone() signal.
    void allTasksDone();

  private:
    QAtomicInt m_activeTasks;
};
