import * as THREE from './threejs-math'

const {
	Matrix4,
	Matrix3,
	Vector4,
	Vector3,
	Vector2,
	Quaternion
} = THREE

//=================================================================================================
Vector2.prototype.toString = function () {
	return `Vector2(${this.x}, ${this.y})`
}

Vector2.prototype.ToArray = function () {
	return [this.x, this.y]
}

Vector3.prototype.toString = function () {
	return `Vector3(${this.x}, ${this.y}, ${this.z})`
}

Vector3.prototype.ToArray = function () {
	return [this.x, this.y, this.z]
}

Vector4.prototype.toString = function () {
	return `Vector4(${this.x}, ${this.y}, ${this.z}, ${this.w})`
}

Vector4.prototype.ToArray = function () {
	return [this.x, this.y, this.z, this.w]
}

Matrix3.prototype.toString = function () {
	const elms = this.elements
	let str = 'Matrix3(\n'
	for (let i = 0; i < 3; i++) {
		for (let j = 0; j < 3; j++) {
			str += '\t' + elms[i * 3 + j]
		}
		str += '\n'
	}
	str += ')'
	return str
}

Matrix3.prototype.ToArray = function () {
	return [...this.elements]
}

Matrix4.prototype.toString = function () {
	const elms = this.elements
	let str = 'Matrix4(\n'
	for (let i = 0; i < 4; i++) {
		for (let j = 0; j < 4; j++) {
			str += '\t' + elms[i * 4 + j]
		}
		str += '\n'
	}
	str += ')'
	return str
}

Matrix4.prototype.ToArray = function () {
	return [...this.elements]
}

Matrix4.prototype.Mul = function (m) {
	return this.multiplyMatrices(m, this);
}

Matrix4.prototype.Multiply = Matrix4.prototype.Mul

Quaternion.prototype.toString = function () {
	return `Vector4(${this._x}, ${this._y}, ${this._z}, ${this._w})`
}

Quaternion.prototype.ToArray = function () {
	return [this._x, this._y, this._z, this._w]
}


//==============================getMethods===================================================
function enumMethods(obj) {
	return `${obj.name}(\n\t${Object.getOwnPropertyNames(obj.prototype).join('\n\t')}\n)`
}

Vector2.enumMethods = function () {
	return enumMethods(this)
}

Vector3.enumMethods = function () {
	return enumMethods(this)
}

Vector4.enumMethods = function () {
	return enumMethods(this)
}

Matrix3.enumMethods = function () {
	return enumMethods(this)
}

Matrix4.enumMethods = function () {
	return enumMethods(this)
}

Quaternion.enumMethods = function () {
	return enumMethods(this)
}

globalThis.Matrix4      = Matrix4
globalThis.Matrix3      = Matrix3
globalThis.Vector4      = Vector4
globalThis.Vector3      = Vector3
globalThis.Vector2      = Vector2
globalThis.Quaternion   = Quaternion