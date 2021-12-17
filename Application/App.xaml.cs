using System.Windows;
using System.Threading;

namespace ToolKitV
{
    public partial class App : Application
    {
        protected Mutex Mutex;
        protected override void OnStartup(StartupEventArgs e)
        {
            Mutex = new Mutex(true, ResourceAssembly.GetName().Name);

            bool canStart = false;

#if DEBUG
            canStart = true;
#else
            for (int i = 0; i != e.Args.Length; ++i)
            {
                if (e.Args[i] == "-startedFromUpdater")
                {
                    canStart = true;
                    break;
                }
            }
#endif
            if (!Mutex.WaitOne(0, false) || !canStart)
            {
                Current.Shutdown();
                return;
            }
            else
            {
                ShutdownMode = ShutdownMode.OnLastWindowClose;
            }
            base.OnStartup(e);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            //if (Mutex != null)
            //{
            //    Mutex.ReleaseMutex();
            //}

            base.OnExit(e);
        }
    }
}
