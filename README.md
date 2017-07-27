## Calculix 2.12
#### Makefiles to build SPOOLES, ARPACK and CalculiX. The scripts automatically pull the software from internet, builds the software and installs it locally. 
-----
##### Necessary to install packages for Ubuntu:
```
sudo apt install gfortran xorg-dev #libxmu-headers
```
##### The script can be run anywhere after downloading:
```
git clone https://github.com/luvres/calculix.git

pushd calculix/
```
##### And will install in ~/CalculiX. The installation is started with
```
./install
```
```
sudo mv /usr/bin/ccx /usr/bin/ccx_`ccx -version | grep 'is' | awk '{print $4}'`
sudo cp $HOME/CalculiX-2.12/bin/ccx_2.12 /usr/bin/ccx
sudo cp $HOME/CalculiX-2.12/bin/cgx /usr/bin/cgx
```
