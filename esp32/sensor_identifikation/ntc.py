import numpy as np
import matplotlib.pyplot as plt
import casadi as cas

# define functions
def R_ntc(R_N, B, T, T_N):
    return R_N * np.exp(B*(1/T-1/T_N))


def U_meas(U_ges, R_1, R_2):

    return U_ges/(R_1+R_2) * R_2

# define measurements constants

R_measured = [103e3, 120e3, 70e3, 15.2e3]
T_measured_C = [25, 20, 33, 74]

R_N = 100e3
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
plt.show()
