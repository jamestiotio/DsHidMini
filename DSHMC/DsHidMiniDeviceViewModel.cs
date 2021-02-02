﻿using System.ComponentModel;
using System.Runtime.CompilerServices;
using DSHMC.Annotations;

namespace DSHMC
{
    class DsHidMiniDeviceViewModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        [NotifyPropertyChangedInvocator]
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
