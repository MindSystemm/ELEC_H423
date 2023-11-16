import matplotlib.pyplot as plt
import time 
class AnimatedChart:
    def __init__(self, name, title_x, title_y):
        self._fig = plt.figure()
        self._plts = {}
        self._name = name
        self._title_x = title_x
        self._title_y = title_y

    def update(self, id, x, y):
        if (id not in self._plts): self.__add_plt(id)

        plt = self._plts[id]

        plt.clear()
        plt.plot(x, y)
    
    def __add_plt(self, id):
        self._plts[id] = self._fig.add_subplot(1,1,1)
        self._plts[id].set_title(self._name)
        self._plts[id].set_xlabel(self._title_x)
        self._plts[id].set_ylabel(self._title_y)

    def show(self):
        self._fig.show()

charts = {
    'temp': AnimatedChart("Temperature", "datetime", "°C"),
    'humi': AnimatedChart("Humidity", "datetime", "humidity")
}

for key in charts:
    charts[key].update("test", [], [])
    charts[key].show()

input("close")