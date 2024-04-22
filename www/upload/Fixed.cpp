/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Fixed.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jy_23 <jy_23@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/26 14:39:35 by jy_23             #+#    #+#             */
/*   Updated: 2023/10/09 15:34:35 by jy_23            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

#include "Fixed.h"

const int Fixed::fractionalBitsNumber = 8;

Fixed::Fixed(){

	std::cout << "Default constructor called" << std::endl;
	fixedPointNumber = 0;
}

Fixed::Fixed(const Fixed &obj){

	std::cout << "Copy constructor called" << std::endl;
	*this = obj;
}

Fixed::~Fixed(){
	
	std::cout << "Destructor called" << std::endl;
}

int Fixed::getRawBits(void) const{

	std::cout << "getRawBits member function called" << std::endl;
	return fixedPointNumber;
}

void Fixed::setRawBits(int const raw){

	// std::cout << "setRawBits member function called" << std::endl;
	fixedPointNumber = raw;
}

Fixed &Fixed::operator=(Fixed const &obj){

	std::cout << "Copy assignment operator called" << std::endl;
	if (this != &obj) {
		this->setRawBits(obj.getRawBits());
	}
	return *this;
}
