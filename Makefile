

all:
	$(CXX) scripts/sysstat.cpp -o scripts/tmux_sysstat


clean:
	rm -rf tmux_sysstat
