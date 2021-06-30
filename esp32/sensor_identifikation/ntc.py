import numpy as np
import matplotlib.pyplot as plt
import casadi as cas

# define functions
def R_ntc(R_N, B, T, T_N):
    return R_N * np.exp(B*(1/T-1/T_N))


def U_meas(U_ges, R_1, R_2):

    return U_ges/(R_1+R_2) * R_2

def Temp(U_meas, R_1, U_ges, B, R_N, T_N):
    R_NTC = U_meas * R_1 / (U_ges - U_meas)
    T_kelvin = 1 / (np.log(R_NTC / R_N) / B + 1 / T_N)
    return T_kelvin - 273.15

def OneProTemp(U_meas, R_1, U_ges, B, R_N, T_N):
    R_NTC = U_meas * R_1 / (U_ges - U_meas)
    return  (np.log(R_NTC / R_N) / B + 1 / T_N)

def Temp_adc(adc_value, R_1, U_ges, B, R_N, T_N):
    return Temp(U_ges/4096 * adc_value, R_1, U_ges, B, R_N, T_N)

# define measurements constants

R_measured = [103e3, 120e3, 70e3, 15.2e3]
T_measured_C = [25, 20, 33, 74]

R_N = 103e3
T_N = 298.15
U_ges = 3.3
T_range_C = [0, 200]
T_Nenn_C = 65
T_Nenn = T_Nenn_C+273.15


# Kelvin from Temp
T_range = [i + 273.15 for i in T_range_C]
T_measured = [i + 273.15 for i in T_measured_C]

# Fit B
B_sym = cas.SX.sym("B")
f = 0
for R, T in zip(R_measured, T_measured):
    f += (R-R_ntc(R_N, B_sym, T, T_N))**2


nlp = {'x': B_sym, 'f': f}
S = cas.nlpsol('S', 'ipopt', nlp)
res = S()

B = res['x'].full()[0]

#Fit R_1
R_1 = cas.SX.sym("R_1")
T_sym = cas.SX.sym("T")

U = U_meas(U_ges, R_1, R_ntc(R_N, B, T_sym, T_N))
jac = cas.Function("dudT", [R_1, T_sym], [cas.jacobian(U, T_sym)])

lbx = [0]
ubx = [cas.inf]
nlp = {'x': R_1, 'f': jac(R_1, T_Nenn)}

S = cas.nlpsol('S', 'ipopt', nlp)
res = S(lbx=lbx, ubx=ubx)

R_1 = res['x'].full()[0]

# plot
T = np.linspace(T_range[0], T_range[1], num=200)
T_C = np.linspace(T_range_C[0], T_range_C[1], num=200)
print("R_1: %e" % R_1)
print("B: %e" % B)
plt.figure()
plt.plot(T_C, R_ntc(R_N, B, T, T_N), label="R_NTC")
plt.plot(T_measured_C, R_measured, "x", label="R_measured")
plt.xlabel("Temp [Grad]")
plt.ylabel("R [Ohm]")
plt.legend()

plt.figure()
U_range = U_meas(U_ges, R_1, R_ntc(R_N, B, T, T_N))
plt.plot(T_C, U_range, label="R/NTC")
plt.vlines(T_Nenn_C, U_range.min(), U_range.max())
plt.xlabel("Temp [Grad]")
plt.ylabel("U [V]")
plt.legend()

# Calibrate Sensor

# measurements

adc_voltage_measured = [2.21634, 2.17343, 1.59904, 1.49781, 0.84637, 0.71773, 0.59042, 0.67618, 0.18674, 0.22936,0.18058,0.16265, 0.31080, 0.35350, 0.51420, 0.54871, 0.69035, 0.74026, 0.93252, 0.97848, 1.14037, 1.22280, 1.62269, 1.66503, 2.68944, 2.70243]
T_measured = [42, 43, 60, 65, 92, 99, 106, 101, 156, 146, 158, 161, 133, 128, 112, 109, 100, 97, 87, 85, 78, 75, 61, 59, 24, 23]

R1 = 24.9e3
B_sym = cas.SX.sym("B")
Rn_sym = cas.SX.sym("Rn")
Tn_sym = cas.SX.sym("Tn")
syms = cas.vertcat(B_sym, Rn_sym, Tn_sym)
x0 = [B, 103e3, T_N]
f = 0
for T, adc_voltage in zip(T_measured, adc_voltage_measured):
    f += (T-Temp(adc_voltage, R1, U_ges, B_sym, Rn_sym, Tn_sym))**2
    
f += (Rn_sym-103e3)**2+(Tn_sym-T_N)**2
nlp = {'x': syms, 'f': f}
S = cas.nlpsol('S', 'ipopt', nlp)
res = S(x0=x0)

B = res['x'].full()[0]
Rn = res['x'].full()[1]
Tn = res['x'].full()[2]

print("B:", B, "R1:", R1, "Rn:", Rn, "Tn:", Tn)

plt.figure()
adc_range = np.linspace(min(adc_voltage_measured), max(adc_voltage_measured))
plt.plot(adc_range, Temp(adc_range, R1, U_ges, B, Rn, Tn), label="Fitted Model")
plt.plot(adc_voltage_measured, T_measured, 'x', label="Measured")
plt.xlabel("ADC voltage [V]")
plt.ylabel("Temp [Grad]")
plt.legend()

plt.show()
